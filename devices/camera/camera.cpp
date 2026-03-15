#include "camera.h"
#include "ui/ui.h"

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <string>
#include <string_view>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {

constexpr int CAMERA_FRAME_WIDTH = 160;
constexpr int CAMERA_FRAME_HEIGHT = 128;
constexpr size_t CAMERA_FRAME_BYTES = CAMERA_FRAME_WIDTH * CAMERA_FRAME_HEIGHT * sizeof(uint16_t);
constexpr size_t CAMERA_STDERR_BUFFER_SIZE = 2048;
constexpr size_t CAMERA_MESSAGE_BUFFER_SIZE = 512;
constexpr const char *CAMERA_CPP_SUFFIX = "/devices/camera/camera.cpp";
constexpr const char *CAMERA_SCRIPTS_SUFFIX = "/scripts/python";

pid_t g_camera_pid = -1;
int g_camera_stdout_fd = -1;
int g_camera_stderr_fd = -1;
char g_camera_selected_script[128] = {0};
char g_camera_stderr_buffer[CAMERA_STDERR_BUFFER_SIZE] = {0};
size_t g_camera_stderr_length = 0;
uint8_t g_camera_frame_buffer[CAMERA_FRAME_BYTES] = {0};
size_t g_camera_frame_length = 0;

std::string build_project_root(void)
{
	std::string source_path = __FILE__;
	size_t suffix_position = source_path.rfind(CAMERA_CPP_SUFFIX);

	if(suffix_position == std::string::npos)
	{
		return std::string(".");
	}

	return source_path.substr(0, suffix_position);
}

const std::string &camera_scripts_path_storage(void)
{
	static const std::string scripts_path = build_project_root() + CAMERA_SCRIPTS_SUFFIX;
	return scripts_path;
}

void camera_close_fd(int *fd)
{
	if(fd != NULL && *fd >= 0)
	{
		close(*fd);
		*fd = -1;
	}
}

void camera_reset_process_state(void)
{
	g_camera_pid = -1;
	camera_close_fd(&g_camera_stdout_fd);
	camera_close_fd(&g_camera_stderr_fd);
	g_camera_frame_length = 0;
	g_camera_stderr_length = 0;
	g_camera_stderr_buffer[0] = '\0';
}

bool camera_has_py_suffix(const char *name)
{
	std::string_view file_name = name ? name : "";

	return file_name.size() > 3 && file_name.substr(file_name.size() - 3) == ".py";
}

void camera_append_stderr(const char *data, size_t length)
{
	if(data == NULL || length == 0)
	{
		return;
	}

	if(length >= CAMERA_STDERR_BUFFER_SIZE)
	{
		data += length - (CAMERA_STDERR_BUFFER_SIZE - 1);
		length = CAMERA_STDERR_BUFFER_SIZE - 1;
		g_camera_stderr_length = 0;
	}

	if(g_camera_stderr_length + length >= CAMERA_STDERR_BUFFER_SIZE)
	{
		size_t overflow = g_camera_stderr_length + length - (CAMERA_STDERR_BUFFER_SIZE - 1);
		memmove(g_camera_stderr_buffer,
				g_camera_stderr_buffer + overflow,
				g_camera_stderr_length - overflow);
		g_camera_stderr_length -= overflow;
	}

	memcpy(g_camera_stderr_buffer + g_camera_stderr_length, data, length);
	g_camera_stderr_length += length;
	g_camera_stderr_buffer[g_camera_stderr_length] = '\0';
}

void camera_set_non_blocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);

	if(flags >= 0)
	{
		fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	}
}

void camera_consume_stdout(void)
{
	uint8_t read_buffer[8192];
	ssize_t read_size;

	while(g_camera_stdout_fd >= 0)
	{
		read_size = read(g_camera_stdout_fd, read_buffer, sizeof(read_buffer));
		if(read_size <= 0)
		{
			break;
		}

		size_t offset = 0;
		while(offset < static_cast<size_t>(read_size))
		{
			size_t chunk_size = CAMERA_FRAME_BYTES - g_camera_frame_length;
			size_t copy_size = std::min(chunk_size, static_cast<size_t>(read_size) - offset);

			memcpy(g_camera_frame_buffer + g_camera_frame_length, read_buffer + offset, copy_size);
			g_camera_frame_length += copy_size;
			offset += copy_size;

			if(g_camera_frame_length == CAMERA_FRAME_BYTES)
			{
				ui_camera_request_frame_update(g_camera_frame_buffer, CAMERA_FRAME_BYTES);
				g_camera_frame_length = 0;
			}
		}
	}
}

void camera_consume_stderr(void)
{
	char read_buffer[256];
	ssize_t read_size;

	while(g_camera_stderr_fd >= 0)
	{
		read_size = read(g_camera_stderr_fd, read_buffer, sizeof(read_buffer) - 1);
		if(read_size <= 0)
		{
			break;
		}

		camera_append_stderr(read_buffer, static_cast<size_t>(read_size));
	}
}

void camera_extract_error_message(char *buffer, size_t buffer_size, int status)
{
	const char *fallback_message = "Python script exited unexpectedly";
	const char *last_line = g_camera_stderr_buffer;
	const char *cursor = g_camera_stderr_buffer;

	if(buffer == NULL || buffer_size == 0)
	{
		return;
	}

	while(cursor != NULL && *cursor != '\0')
	{
		const char *next = strchr(cursor, '\n');
		if(cursor[0] != '\0' && cursor[0] != '\n' && cursor[0] != '\r')
		{
			last_line = cursor;
		}

		if(next == NULL)
		{
			break;
		}

		cursor = next + 1;
	}

	if(last_line != NULL && last_line[0] != '\0')
	{
		snprintf(buffer, buffer_size, "%s", last_line);
		char *newline = strpbrk(buffer, "\r\n");
		if(newline != NULL)
		{
			*newline = '\0';
		}

		if(buffer[0] != '\0')
		{
			return;
		}
	}

	if(WIFSIGNALED(status))
	{
		snprintf(buffer,
				 buffer_size,
				 "Python script terminated by signal %d",
				 WTERMSIG(status));
		return;
	}

	if(WIFEXITED(status))
	{
		snprintf(buffer,
				 buffer_size,
				 "Python script exited with code %d",
				 WEXITSTATUS(status));
		return;
	}

	snprintf(buffer, buffer_size, "%s", fallback_message);
}

void camera_finish_process(bool show_error, int status)
{
	char message[CAMERA_MESSAGE_BUFFER_SIZE];

	if(show_error)
	{
		camera_extract_error_message(message, sizeof(message), status);
		ui_camera_request_show_error(message);
	}
	else
	{
		ui_camera_request_viewer_close();
	}

	camera_reset_process_state();
}

}  // namespace

const char *camera_get_scripts_path(void)
{
	return camera_scripts_path_storage().c_str();
}

int camera_scan_scripts(char *options, size_t buffer_size)
{
	DIR *dir = NULL;
	struct dirent *entry = NULL;
	std::vector<std::string> scripts;
	size_t used = 0;

	if(options == NULL || buffer_size == 0)
	{
		return -1;
	}

	options[0] = '\0';
	dir = opendir(camera_get_scripts_path());
	if(dir == NULL)
	{
		snprintf(options, buffer_size, "No Python Files");
		return -1;
	}

	while((entry = readdir(dir)) != NULL)
	{
		if(entry->d_name[0] == '.')
		{
			continue;
		}

		if(camera_has_py_suffix(entry->d_name))
		{
			scripts.emplace_back(entry->d_name);
		}
	}

	closedir(dir);
	std::sort(scripts.begin(), scripts.end());

	if(scripts.empty())
	{
		snprintf(options, buffer_size, "No Python Files");
		return 0;
	}

	for(size_t index = 0; index < scripts.size(); ++index)
	{
		int written = snprintf(options + used,
							   buffer_size - used,
							   "%s%s",
							   index == 0 ? "" : "\n",
							   scripts[index].c_str());
		if(written < 0 || static_cast<size_t>(written) >= buffer_size - used)
		{
			break;
		}

		used += static_cast<size_t>(written);
	}

	return 0;
}

void camera_set_selected_script(const char *script_name)
{
	snprintf(g_camera_selected_script,
			 sizeof(g_camera_selected_script),
			 "%s",
			 script_name ? script_name : "");
}

const char *camera_get_selected_script(void)
{
	return g_camera_selected_script;
}

void camera_clear_selected_script(void)
{
	g_camera_selected_script[0] = '\0';
}

bool camera_start_script_process(const char *script_name, char *error, size_t error_size)
{
	int stdout_pipe[2] = {-1, -1};
	int stderr_pipe[2] = {-1, -1};
	pid_t pid;

	if(script_name == NULL || script_name[0] == '\0' || !camera_has_py_suffix(script_name))
	{
		snprintf(error, error_size, "Invalid Python script");
		return false;
	}

	if(pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0)
	{
		snprintf(error, error_size, "Failed to create script pipes: %s", strerror(errno));
		camera_close_fd(&stdout_pipe[0]);
		camera_close_fd(&stdout_pipe[1]);
		camera_close_fd(&stderr_pipe[0]);
		camera_close_fd(&stderr_pipe[1]);
		return false;
	}

	pid = fork();
	if(pid < 0)
	{
		snprintf(error, error_size, "Failed to fork Python process: %s", strerror(errno));
		camera_close_fd(&stdout_pipe[0]);
		camera_close_fd(&stdout_pipe[1]);
		camera_close_fd(&stderr_pipe[0]);
		camera_close_fd(&stderr_pipe[1]);
		return false;
	}

	if(pid == 0)
	{
		int stdin_fd = open("/dev/null", O_RDONLY);

		chdir(camera_get_scripts_path());
		dup2(stdout_pipe[1], STDOUT_FILENO);
		dup2(stderr_pipe[1], STDERR_FILENO);
		if(stdin_fd >= 0)
		{
			dup2(stdin_fd, STDIN_FILENO);
			close(stdin_fd);
		}

		close(stdout_pipe[0]);
		close(stdout_pipe[1]);
		close(stderr_pipe[0]);
		close(stderr_pipe[1]);
		setenv("PYTHONUNBUFFERED", "1", 1);
		execlp("python3", "python3", "-u", script_name, (char *)NULL);
		dprintf(STDERR_FILENO, "Failed to execute %s: %s\n", script_name, strerror(errno));
		_exit(127);
	}

	close(stdout_pipe[1]);
	close(stderr_pipe[1]);
	camera_set_non_blocking(stdout_pipe[0]);
	camera_set_non_blocking(stderr_pipe[0]);

	g_camera_pid = pid;
	g_camera_stdout_fd = stdout_pipe[0];
	g_camera_stderr_fd = stderr_pipe[0];
	g_camera_frame_length = 0;
	g_camera_stderr_length = 0;
	g_camera_stderr_buffer[0] = '\0';
	camera_set_selected_script(script_name);
	return true;
}

void camera_poll_script_process(void)
{
	fd_set read_set;
	struct timeval timeout;
	int max_fd = -1;
	int ready = 0;
	int status = 0;
	pid_t wait_result;

	if(g_camera_pid <= 0)
	{
		return;
	}

	FD_ZERO(&read_set);
	if(g_camera_stdout_fd >= 0)
	{
		FD_SET(g_camera_stdout_fd, &read_set);
		max_fd = std::max(max_fd, g_camera_stdout_fd);
	}

	if(g_camera_stderr_fd >= 0)
	{
		FD_SET(g_camera_stderr_fd, &read_set);
		max_fd = std::max(max_fd, g_camera_stderr_fd);
	}

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	if(max_fd >= 0)
	{
		ready = select(max_fd + 1, &read_set, NULL, NULL, &timeout);
		if(ready > 0)
		{
			if(g_camera_stdout_fd >= 0 && FD_ISSET(g_camera_stdout_fd, &read_set))
			{
				camera_consume_stdout();
			}

			if(g_camera_stderr_fd >= 0 && FD_ISSET(g_camera_stderr_fd, &read_set))
			{
				camera_consume_stderr();
			}
		}
	}

	wait_result = waitpid(g_camera_pid, &status, WNOHANG);
	if(wait_result == g_camera_pid)
	{
		camera_finish_process(!(WIFEXITED(status) && WEXITSTATUS(status) == 0), status);
	}
}

void camera_stop_script_process(void)
{
	int status = 0;

	if(g_camera_pid <= 0)
	{
		return;
	}

	kill(g_camera_pid, SIGTERM);
	usleep(100000);

	if(waitpid(g_camera_pid, &status, WNOHANG) == 0)
	{
		kill(g_camera_pid, SIGKILL);
		waitpid(g_camera_pid, &status, 0);
	}

	camera_reset_process_state();
}

bool camera_is_script_running(void)
{
	return g_camera_pid > 0;
}