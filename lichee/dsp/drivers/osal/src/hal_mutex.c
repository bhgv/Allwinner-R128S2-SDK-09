#include <hal_osal.h>
#include <hal_mutex.h>
#include <hal_interrupt.h>
#include <hal_status.h>

int hal_mutex_init(hal_mutex_t mutex)
{
	mutex->ptr = xSemaphoreCreateMutexStatic(&mutex->entry);
	if (mutex->ptr == NULL) {
		return HAL_ERROR;
	}
	return HAL_OK;
}

int hal_mutex_detach(hal_mutex_t mutex)
{
    mutex->ptr = NULL;
	return HAL_OK;
}

hal_mutex_t hal_mutex_create(void)
{
	/* default not support recursive */
    hal_mutex_t mutex = hal_malloc(sizeof(*mutex));
    if (!mutex)
        return NULL;
	hal_mutex_init(mutex);
	return mutex;
}

int hal_mutex_delete(hal_mutex_t mutex)
{
	if (mutex == NULL || mutex->ptr == NULL) {
		return HAL_INVALID;
	}
	vSemaphoreDelete(mutex->ptr);
    hal_free(mutex);
    mutex = NULL;
	return 0;
}

int hal_mutex_unlock(hal_mutex_t mutex)
{
	BaseType_t ret;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if (mutex == NULL || mutex->ptr == NULL) {
		return HAL_INVALID;
	}

	if (hal_interrupt_get_nest()) {
		ret = xSemaphoreGiveFromISR(mutex->ptr, &xHigherPriorityTaskWoken);
		if (ret == pdPASS) {
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		}
	}
	else
		ret = xSemaphoreGive(mutex->ptr);

	if (ret != pdPASS) {
		return HAL_ERROR;
	}

	return HAL_OK;
}

int hal_mutex_timedwait(hal_mutex_t mutex, int ticks)
{
	TickType_t xDelay = (TickType_t)ticks;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	BaseType_t ret;

	if (mutex == NULL || mutex->ptr == NULL) {
		return HAL_INVALID;
	}

	if (hal_interrupt_get_nest()) {
		ret = xSemaphoreTakeFromISR(mutex->ptr, &xHigherPriorityTaskWoken);
		if (ret == pdPASS) {
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		}
	}
	else
		ret = xSemaphoreTake(mutex->ptr, xDelay);

	if (ret != pdPASS)
		return HAL_ERROR;

	return HAL_OK;
}

int hal_mutex_lock(hal_mutex_t mutex)
{
	TickType_t xDelay = portMAX_DELAY;

	return hal_mutex_timedwait(mutex, xDelay);
}

int hal_mutex_trylock(hal_mutex_t mutex)
{
	TickType_t xDelay = 0;

	return hal_mutex_timedwait(mutex, xDelay);
}

