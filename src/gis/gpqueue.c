#include <glib.h>
#include "gpqueue.h"

/**
 * SECTION:priority_queues
 * @short_description: a collection of data entries with associated priority
 * values that returns entries one by one in order of priority
 * 
 * <para>
 * The #GPQueue structure and its associated functions provide a sorted
 * collection of objects. Entries can be inserted in any order and at any time,
 * and an entry's priority can be changed after it has been inserted into the
 * queue. Entries are supposed to be removed one at a time in order of priority
 * with g_pqueue_pop(), but deleting entries out of order is possible.
 * </para>
 * <para>
 * The entries <emphasis>cannot</emphasis> be iterated over in any way other
 * than removing them one by one in order of priority, but when doing that,
 * this structure is far more efficient than sorted lists or balanced trees,
 * which on the other hand do not suffer from this restriction.
 * </para>
 * <para>
 * You will want to be very careful with calls that use #GPQueueHandle.
 * Handles immediately become invalid when an entry is removed from a #GPQueue,
 * but the current implementation cannot detect this and will do unfortunate
 * things to undefined memory locations if you try to use an invalid handle.
 * </para>
 * <note>
 *   <para>
 *     Internally, #GPQueue currently uses a Fibonacci heap to store
 *     the entries. This implementation detail may change.
 *   </para>
 * </note>
 **/

struct _GPQueueNode {
  GPQueueNode *next;
  GPQueueNode *prev;
  GPQueueNode *parent;
  GPQueueNode *child;

  gpointer data;

  gint degree;
  gboolean marked;
};

struct _GPQueue {
  GPQueueNode *root;
  GCompareDataFunc cmp;
  gpointer *cmpdata;
};

/**
 * g_pqueue_new:
 * @compare_func: the #GCompareDataFunc used to sort the new priority queue.
 *   This function is passed two elements of the queue and should return 0 if
 *   they are equal, a negative value if the first comes before the second, and
 *   a positive value if the second comes before the first.
 * @compare_userdata: user data passed to @compare_func
 * 
 * Creates a new #GPQueue.
 * 
 * Returns: a new #GPQueue.
 * 
 * Since: 2.x
 **/
GPQueue*
g_pqueue_new (GCompareDataFunc compare_func,
              gpointer *compare_userdata)
{
  g_return_val_if_fail (compare_func != NULL, NULL);

  GPQueue *pqueue = g_slice_new (GPQueue);
  pqueue->root = NULL;
  pqueue->cmp = compare_func;
  pqueue->cmpdata = compare_userdata;
  return pqueue;
}

/**
 * g_pqueue_is_empty:
 * @pqueue: a #GPQueue.
 * 
 * Returns %TRUE if the queue is empty.
 * 
 * Returns: %TRUE if the queue is empty.
 * 
 * Since: 2.x
 **/
gboolean
g_pqueue_is_empty (GPQueue *pqueue)
{
  return (pqueue->root == NULL);
}

static void
g_pqueue_node_foreach (GPQueueNode *node,
                       GPQueueNode *stop,
                       GFunc func,
	               gpointer user_data)
{
  if (node == NULL || node == stop) return;
  func(node->data, user_data);
  if (stop == NULL) stop = node;
  g_pqueue_node_foreach (node->next,  stop, func, user_data);
  g_pqueue_node_foreach (node->child, NULL, func, user_data);
}

/**
 * g_pqueue_foreach:
 * @pqueue: a #GQueue.
 * @func: the function to call for each element's data
 * @user_data: user data to pass to func
 *
 * Calls func for each element in the pqueue passing user_data to the function.
 * 
 * Since: 2.x
 */
void
g_pqueue_foreach (GPQueue *pqueue,
                  GFunc func,
		  gpointer user_data)
{
  g_pqueue_node_foreach (pqueue->root, NULL, func, user_data);
}

static void
g_pqueue_add_ptr_cb (gpointer obj, GPtrArray *ptrs)
{
	g_ptr_array_add(ptrs, obj);
}
/**
 * g_pqueue_get_array:
 * @pqueue: a #GQueue.
 *
 * Construct a GPtrArray for the items in pqueue. This can be useful when
 * updating the priorities of all the elements in pqueue.
 *
 * Returns: A GPtrArray containing a pointer to each item in pqueue
 * 
 * Since: 2.x
 */
GPtrArray *
g_pqueue_get_array (GPQueue *pqueue)
{
	GPtrArray *ptrs = g_ptr_array_new();
	g_pqueue_foreach(pqueue, (GFunc)g_pqueue_add_ptr_cb, ptrs);
	return ptrs;
}

static inline gint
cmp (GPQueue *pqueue,
     GPQueueNode *a,
     GPQueueNode *b)
{
  return pqueue->cmp (a->data, b->data, pqueue->cmpdata);
}

static inline void
g_pqueue_node_cut (GPQueueNode *src)
{
  src->prev->next = src->next;
  src->next->prev = src->prev;
  src->next = src;
  src->prev = src;
}

static inline void
g_pqueue_node_insert_before (GPQueueNode *dest,
                             GPQueueNode *src)
{
  GPQueueNode *prev;

  prev = dest->prev;
  dest->prev = src->prev;
  src->prev->next = dest;
  src->prev = prev;
  prev->next = src;
}

static inline void
g_pqueue_node_insert_after (GPQueueNode *dest,
                            GPQueueNode *src)
{
  GPQueueNode *next;

  next = dest->next;
  dest->next = src;
  src->prev->next = next;
  next->prev = src->prev;
  src->prev = dest;
}

/**
 * g_pqueue_push:
 * @pqueue: a #GPQueue.
 * @data: the object to insert into the priority queue.
 * 
 * Inserts a new entry into a #GPQueue.
 * 
 * The returned handle can be used in calls to g_pqueue_remove() and
 * g_pqueue_priority_changed(). Never make such calls for entries that have
 * already been removed from the queue. The same @data can be inserted into
 * a #GPQueue more than once, but remember that in this case,
 * g_pqueue_priority_changed() needs to be called for
 * <emphasis>every</emphasis> handle for that object if its priority changes.
 * 
 * Returns: a handle for the freshly inserted entry.
 * 
 * Since: 2.x
 **/
GPQueueHandle
g_pqueue_push (GPQueue *pqueue,
               gpointer data)
{
  GPQueueNode *e;

  e = g_slice_new (GPQueueNode);
  e->next = e;
  e->prev = e;
  e->parent = NULL;
  e->child = NULL;
  e->data = data;
  e->degree = 0;
  e->marked = FALSE;

  if (pqueue->root != NULL) {
    g_pqueue_node_insert_before (pqueue->root, e);
    if (cmp (pqueue, e, pqueue->root) < 0)
      pqueue->root = e;
  } else {
    pqueue->root = e;
  }

  return e;
}

/**
 * g_pqueue_peek:
 * @pqueue: a #GPQueue.
 * 
 * Returns the topmost entry's data pointer, or %NULL if the queue is empty.
 * 
 * If you need to tell the difference between an empty queue and a queue
 * that happens to have a %NULL pointer at the top, check if the queue is
 * empty first.
 * 
 * Returns: the topmost entry's data pointer, or %NULL if the queue is empty.
 * 
 * Since: 2.x
 **/
gpointer
g_pqueue_peek (GPQueue *pqueue)
{
  return (pqueue->root != NULL) ? pqueue->root->data : NULL;
}

static inline GPQueueNode*
g_pqueue_make_child (GPQueueNode *a,
                     GPQueueNode *b)
{
  g_pqueue_node_cut(b);
  if (a->child != NULL) {
    g_pqueue_node_insert_before (a->child, b);
    a->degree += 1;
  } else {
    a->child = b;
    a->degree = 1;
  }
  b->parent = a;
  return a;
}

static inline GPQueueNode*
g_pqueue_join_trees (GPQueue *pqueue,
                     GPQueueNode *a,
                     GPQueueNode *b)
{
  if (cmp (pqueue, a, b) < 0)
    return g_pqueue_make_child (a, b);
  return g_pqueue_make_child (b, a);
}

static void
g_pqueue_fix_rootlist (GPQueue* pqueue)
{
  gsize degnode_size;
  GPQueueNode **degnode;
  GPQueueNode sentinel;
  GPQueueNode *current;
  GPQueueNode *minimum;

  /* We need to iterate over the circular list we are given and do
   * several things:
   * - Make sure all the elements are unmarked
   * - Make sure to return the element in the list with smallest
   *   priority value
   * - Find elements of identical degree and join them into trees
   * The last point is irrelevant for correctness, but essential
   * for performance. If we did not do this, our data structure would
   * degrade into an unsorted linked list.
   */

  degnode_size = (8 * sizeof(gpointer) + 1) * sizeof(gpointer);
  degnode = g_slice_alloc0 (degnode_size);

  sentinel.next = &sentinel;
  sentinel.prev = &sentinel;
  g_pqueue_node_insert_before (pqueue->root, &sentinel);

  current = pqueue->root;
  while (current != &sentinel) {
    current->marked = FALSE;
    current->parent = NULL;
    gint d = current->degree;
    if (degnode[d] == NULL) {
      degnode[d] = current;
      current = current->next;
    } else {
      if (degnode[d] != current) {
        current = g_pqueue_join_trees (pqueue, degnode[d], current);
        degnode[d] = NULL;
      } else {
        current = current->next;
      }
    }
  }

  current = sentinel.next;
  minimum = current;
  while (current != &sentinel) {
    if (cmp (pqueue, current, minimum) < 0)
      minimum = current;
    current = current->next;
  }
  pqueue->root = minimum;

  g_pqueue_node_cut (&sentinel);

  g_slice_free1 (degnode_size, degnode);
}

static void
g_pqueue_remove_root (GPQueue *pqueue,
                      GPQueueNode *root)
{
  /* This removes a node at the root _level_ of the structure, which can be,
   * but does not have to be, the actual pqueue->root node. That is why
   * we require an explicit pointer to the node to be removed instead of just
   * removing pqueue->root implictly.
   */

  /* Step one:
   * If root has any children, pull them up to root level.
   * At this time, we only deal with their next/prev pointers,
   * further changes are made later in g_pqueue_fix_rootlist().
   */
  if (root->child) {
    g_pqueue_node_insert_after (root, root->child);
    root->child = NULL;
    root->degree = 0;
  }

  /* Step two:
   * Cut root out of the list.
   */
  if (root->next != root) {
    pqueue->root = root->next;
    g_pqueue_node_cut (root);
    /* Step three:
     * Clean up the remaining list.
     */
    g_pqueue_fix_rootlist (pqueue);
  } else {
    pqueue->root = NULL;
  }

  g_slice_free (GPQueueNode, root);
}

/**
 * g_pqueue_pop:
 * @pqueue: a #GPQueue.
 * 
 * Removes the topmost entry from a #GPQueue and returns its data pointer.
 * Calling this on an empty #GPQueue is not an error, but removes nothing
 * and returns %NULL.
 * 
 * If you need to tell the difference between an empty queue and a queue
 * that happens to have a %NULL pointer at the top, check if the queue is
 * empty first.
 * 
 * Returns: the topmost entry's data pointer, or %NULL if the queue was empty.
 * 
 * Since: 2.x
 **/
gpointer
g_pqueue_pop (GPQueue *pqueue)
{
  gpointer data;

  if (pqueue->root == NULL) return NULL;
  data = pqueue->root->data;
  g_pqueue_remove_root (pqueue, pqueue->root);
  return data;
}

static inline void
g_pqueue_make_root (GPQueue *pqueue,
                    GPQueueNode *entry)
{
  /* This moves a node up to the root _level_ of the structure.
   * It does not always become the actual root element (pqueue->root).
   */

  GPQueueNode *parent;

  parent = entry->parent;
  entry->parent = NULL;
  entry->marked = FALSE;
  if (parent != NULL) {
    if (entry->next != entry) {
      if (parent->child == entry) parent->child = entry->next;
      g_pqueue_node_cut (entry);
      parent->degree -= 1;
    } else {
      parent->child = NULL;
      parent->degree = 0;
    }
    g_pqueue_node_insert_before (pqueue->root, entry);
  }

  if (cmp (pqueue, entry, pqueue->root) < 0)
    pqueue->root = entry;
}

static void
g_pqueue_cut_tree (GPQueue *pqueue,
                   GPQueueNode *entry)
{
  /* This function moves an entry up to the root level of the structure.
   * It extends g_pqueue_make_root() in that the entry's parent, grandparent
   * etc. may also be moved to the root level if they are "marked". This is
   * not essential for correctness, it just maintains the so-called "potential"
   * of the structure, which is necessary for the amortized runtime analysis.
   */

  GPQueueNode *current;
  GPQueueNode *parent;

  current = entry;
  while ((current != NULL) && (current->parent != NULL)) {
    parent = current->parent;
    g_pqueue_make_root (pqueue, entry);
    if (parent->marked) {
      current = parent;
    } else {
      parent->marked = TRUE;
      current = NULL;
    }
  }
  if (cmp (pqueue, entry, pqueue->root) < 0)
    pqueue->root = entry;
}

/**
 * g_pqueue_remove:
 * @pqueue: a #GPQueue.
 * @entry: a #GPQueueHandle for an entry in @pqueue.
 * 
 * Removes one entry from a #GPQueue.
 * 
 * Make sure that @entry refers to an entry that is actually part of
 * @pqueue at the time, otherwise the behavior of this function is
 * undefined (expect crashes).
 * 
 * Since: 2.x
 **/
void
g_pqueue_remove (GPQueue* pqueue,
                 GPQueueHandle entry)
{
  g_pqueue_cut_tree (pqueue, entry);
  g_pqueue_remove_root (pqueue, entry);
}

/**
 * g_pqueue_priority_changed:
 * @pqueue: a #GPQueue.
 * @entry: a #GPQueueHandle for an entry in @pqueue.
 * 
 * Notifies the #GPQueue that the priority of one entry has changed.
 * The internal representation is updated accordingly.
 * 
 * Make sure that @entry refers to an entry that is actually part of
 * @pqueue at the time, otherwise the behavior of this function is
 * undefined (expect crashes).
 * 
 * Do not attempt to change the priorities of several entries at once.
 * Every time a single object is changed, the #GPQueue needs to be updated
 * by calling g_pqueue_priority_changed() for that object.
 * 
 * Since: 2.x
 **/
void
g_pqueue_priority_changed (GPQueue* pqueue,
                           GPQueueHandle entry)
{
  g_pqueue_cut_tree (pqueue, entry);

  if (entry->child) {
    g_pqueue_node_insert_after (entry, entry->child);
    entry->child = NULL;
    entry->degree = 0;
  }

  g_pqueue_fix_rootlist (pqueue);
}

/**
 * g_pqueue_priority_decreased:
 * @pqueue: a #GPQueue.
 * @entry: a #GPQueueHandle for an entry in @pqueue.
 * 
 * Notifies the #GPQueue that the priority of one entry has
 * <emphasis>decreased</emphasis>.
 * 
 * This is a special case of g_pqueue_priority_changed(). If you are absolutely
 * sure that the new priority of @entry is lower than it was before, you
 * may call this function instead of g_pqueue_priority_changed().
 * 
 * <note>
 *   <para>
 *     In the current implementation, an expensive step in
 *     g_pqueue_priority_changed() can be skipped if the new priority is known
 *     to be lower, leading to an amortized running time of O(1) instead of
 *     O(log n). Of course, if the priority is not actually lower, behavior
 *     is undefined.
 *   </para>
 * </note>
 * 
 * Since: 2.x
 **/
void
g_pqueue_priority_decreased (GPQueue* pqueue,
                             GPQueueHandle entry)
{
  g_pqueue_cut_tree (pqueue, entry);
}

static void
g_pqueue_node_free_all (GPQueueNode *node)
{
  if (node == NULL) return;
  g_pqueue_node_free_all (node->child);
  node->prev->next = NULL;
  g_pqueue_node_free_all (node->next);
  g_slice_free (GPQueueNode, node);
}

/**
 * g_pqueue_clear:
 * @pqueue: a #GPQueue.
 * 
 * Removes all entries from a @pqueue.
 * 
 * Since: 2.x
 **/
void
g_pqueue_clear (GPQueue* pqueue)
{
  g_pqueue_node_free_all (pqueue->root);
  pqueue->root = NULL;
}

/**
 * g_pqueue_free:
 * @pqueue: a #GPQueue.
 * 
 * Deallocates the memory used by @pqueue itself, but not any memory pointed
 * to by the data pointers of its entries.
 * 
 * Since: 2.x
 **/
void
g_pqueue_free (GPQueue* pqueue)
{
  g_pqueue_clear (pqueue);
  g_slice_free (GPQueue, pqueue);
}
