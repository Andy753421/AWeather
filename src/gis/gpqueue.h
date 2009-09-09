#if defined(G_DISABLE_SINGLE_INCLUDES) && !defined (__GLIB_H_INSIDE__) && !defined (GLIB_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#ifndef __G_PQUEUE_H__
#define __G_PQUEUE_H__

G_BEGIN_DECLS

typedef struct _GPQueueNode GPQueueNode;

/**
 * GPQueue:
 * 
 * An opaque structure representing a priority queue.
 * 
 * Since: 2.x
 **/
typedef struct _GPQueue GPQueue;

/**
 * GPQueueHandle:
 * 
 * An opaque value representing one entry in a #GPQueue.
 * 
 * Since: 2.x
 **/
typedef GPQueueNode* GPQueueHandle;

GPQueue*	g_pqueue_new			(GCompareDataFunc compare_func,
						 gpointer *compare_userdata);

void		g_pqueue_free			(GPQueue* pqueue);

gboolean	g_pqueue_is_empty		(GPQueue *pqueue);

void		g_pqueue_foreach		(GPQueue *pqueue,
						 GFunc func,
						 gpointer user_data);

GPtrArray*	g_pqueue_get_array		(GPQueue *pqueue);

GPQueueHandle	g_pqueue_push			(GPQueue *pqueue,
						 gpointer data);

gpointer	g_pqueue_peek			(GPQueue *pqueue);

gpointer	g_pqueue_pop			(GPQueue *pqueue);

void		g_pqueue_remove			(GPQueue* pqueue,
						 GPQueueHandle entry);

void		g_pqueue_priority_changed	(GPQueue* pqueue,
						 GPQueueHandle entry);

void		g_pqueue_priority_decreased	(GPQueue* pqueue,
						 GPQueueHandle entry);

void		g_pqueue_clear			(GPQueue* pqueue);

G_END_DECLS

#endif /* __G_PQUEUE_H__ */
