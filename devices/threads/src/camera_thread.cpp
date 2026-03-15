#include "../threads_conf.h"

#include "ui/ui.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>

bool camera_thread_is_running = false;
bool camera_thread_stop_requested = false;
bool camera_thread_cleanup_running = false;
bool camera_thread_restart_requested = false;
bool camera_thread_is_refresh = false;
bool camera_thread_is_run = false;
bool camera_thread_is_stop_script = false;
char camera_script_name[128];
pthread_t thread_camera;
pthread_t thread_camera_cleanup;
pthread_mutex_t camera_thread_state_mutex = PTHREAD_MUTEX_INITIALIZER;

static void camera_reset_state_locked(void)
{
	camera_thread_is_refresh = false;
	camera_thread_is_run = false;
	camera_thread_is_stop_script = false;
	camera_thread_stop_requested = false;
	camera_script_name[0] = '\0';
}

static void *camera_cleanup_thread(void *arg)
{
	bool should_restart = false;

	(void)arg;

	pthread_join(thread_camera, NULL);

	pthread_mutex_lock(&camera_thread_state_mutex);
	camera_thread_is_running = false;
	should_restart = camera_thread_restart_requested;
	camera_thread_restart_requested = false;
	camera_thread_cleanup_running = false;
	camera_reset_state_locked();
	pthread_mutex_unlock(&camera_thread_state_mutex);

	if(should_restart)
	{
		camera_create_thread();
	}

	return NULL;
}

void camera_create_thread(void)
{
	pthread_mutex_lock(&camera_thread_state_mutex);

	if(camera_thread_is_running)
	{
		if(camera_thread_stop_requested || camera_thread_cleanup_running)
		{
			camera_thread_restart_requested = true;
		}

		pthread_mutex_unlock(&camera_thread_state_mutex);
		return;
	}

	camera_reset_state_locked();
	camera_thread_restart_requested = false;
	camera_thread_cleanup_running = false;
	camera_thread_is_running = true;
	pthread_mutex_unlock(&camera_thread_state_mutex);

	pthread_create(&thread_camera, NULL, camera_thread, NULL);
}

void camera_destroy_thread(void)
{
	pthread_mutex_lock(&camera_thread_state_mutex);

	if(!camera_thread_is_running || camera_thread_cleanup_running)
	{
		pthread_mutex_unlock(&camera_thread_state_mutex);
		return;
	}

	camera_thread_stop_requested = true;
	camera_thread_cleanup_running = true;
	camera_thread_restart_requested = false;
	pthread_mutex_unlock(&camera_thread_state_mutex);

	pthread_create(&thread_camera_cleanup, NULL, camera_cleanup_thread, NULL);
	pthread_detach(thread_camera_cleanup);
}

void *camera_thread(void *arg)
{
	char script_options[512];
	char error_message[512];

	(void)arg;

	ui_camera_request_set_title(camera_get_scripts_path());
	camera_thread_is_refresh = false;

	while(true)
	{
		pthread_mutex_lock(&camera_thread_state_mutex);
		bool should_stop = camera_thread_stop_requested;
		pthread_mutex_unlock(&camera_thread_state_mutex);

		if(should_stop)
		{
			break;
		}

		if(!camera_thread_is_refresh)
		{
			script_options[0] = '\0';
			camera_scan_scripts(script_options, sizeof(script_options));
			ui_camera_request_set_script_list(script_options);
			camera_thread_is_refresh = true;
		}

		if(camera_thread_is_stop_script)
		{
			camera_stop_script_process();
			camera_thread_is_stop_script = false;
		}

		if(camera_thread_is_run)
		{
			error_message[0] = '\0';
			camera_stop_script_process();

			if(!camera_start_script_process(camera_script_name, error_message, sizeof(error_message)))
			{
				ui_camera_request_show_error(error_message);
			}

			camera_thread_is_run = false;
		}

		camera_poll_script_process();
		usleep(1000);
	}

	camera_stop_script_process();
	return NULL;
}

void camera_isrefresh(void)
{
	if(camera_thread_is_refresh)
	{
		camera_thread_is_refresh = false;
	}
}

void camera_isrun(void)
{
	camera_thread_is_stop_script = false;
	camera_thread_is_run = true;
}

void camera_isstop(void)
{
	camera_thread_is_run = false;
	camera_thread_is_stop_script = true;
}

void camera_get_script(const char *script_name)
{
	snprintf(camera_script_name, sizeof(camera_script_name), "%s", script_name ? script_name : "");
}