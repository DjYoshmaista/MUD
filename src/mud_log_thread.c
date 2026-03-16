#include "mud_log_thread.h"
#include "mud_log_sink.h"
#include "mud_utils.h"
#include "mud_log_internal.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>


// --- Queue Definition (opaque struct from header) ---------------------------------------------------------------------

struct MudLogQueue {
    MudLogQueueEntry* entries,      // Ring buffer array
    size_t capacity,                // Total slots
    size_t head,                    // Consumer reads from here (index)
    size_t tail,                    // Producer writes here (index)
    size_t count,                   // Current number of entries

    pthread_mutex_t mutex,          // Protects all fields above
    pthread_cond_t not_empty,       // Consumer waits on this when queue is empty
    pthread_cond_t not_full,        // Producer waits on this when queue is full
};

// ---- Module State ----------------------------------------------------------------------------------------------------

static struct {
    MudLogQueue*    queue,
    pthread_t       thread,
    bool            running,            // Accessed only under queue mutex or after join
} g_log_thread = {0};

// ---- Queue Operations -------------------------------------------------------------------------------------------------

// Create ring buffer and initialize synchronization primitives
// Returns NULL on failure
static MudLogQueue* queue_create(size_t capacity) {
    // TODO: Implement
    //      - malloc the MudLogQueue struct
    MudLogQueue* queue = malloc(sizeof(MudLogQueue));
    if (queue == NULL) {
        return NULL;
        // TODO: Handle failure
        //      - pthread_mutex_init
        //      - pthread_cond_init (both condvers)
        //      - Initialize head=0, tail=0, count=0
        //      - On any failure, clean up partial allocations and return NULL
    }

    //      - malloc the entries array (capacity * sizeof(MudLogQueueEntry))
    queue->entries = malloc(capacity * sizeof(MudLogQueueEntry));
    if (queue->entries == NULL) {
        free(queue);
        return NULL;
        // TODO: Handle failure
        //      - pthread_mutex_init
        //      - pthread_cond_init (both condvers)
        //      - Initialize head=0, tail=0, count=0
        //      - On any failure, clean up partial allocations and return NULL
    }

    //      - pthread_mutex_init
    pthread_mutex_init(&queue->mutex, NULL);
    //      - pthread_cond_init (both condvers)
    pthread_cond_init(&queue->not_empty, NULL);
    //      - Initialize head=0, tail=0, count=0
    pthread_cond_init(&queue->not_full, NULL);

    queue->capacity = capacity;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;

    return queue;
    //      - On any failure, clean up partial allocations and return NULL
}

// Destroy the queue.  Caller must ensure no threads are using it.
static void queue_destroy(MudLogQueue* queue) {
    // TODO: Implement
    assert(queue != NULL && "queue_destroy called with NULL");
    if (queue == NULL) {
        fprintf(stderr, "[WARN]mud_log_thread.c: queue_destroy called with NULL\n");
        return;
    }

    //      - pthread_cond_destroy (both)
    rc = pthread_cond_destroy(&queue->not_empty);
    assert(rc == 0 && "failed to destroy not_empty condvar");
    rc = pthread_cond_destroy(&queue->not_full);
    assert(rc == 0 && "failed to destroy not_full condvar");
    //      - pthread_mutex_destroy
    int rc = pthread_mutex_destroy(&queue->mutex);
    if (rc == EBUSY) {
        // Lock still held - bug in shutdown sequencing (try to unlock and retry rather than leaking the mutex)
        pthread_mutex_unlock(&queue->mutex);
        rc = pthread_mutex_destroy(&queue->mutex);
    }
    if (rc != 0) {
        fprintf(stderr, "[WARN]mud_log_thread.c: failed to destroy queue mutex: %s\n", strerror(rc));
    }

    //      - free entries array
    if (queue->entries != NULL) {
        memset(queue->entries, 0xDD, queue->capacity * sizeof(MudLogQueueEntry));
        free(queue->entries);
        queue->entries = NULL;
    }
    //      - free queue struct
    memset(queue, 0xDD, sizeof(MudLogQueue));
    free(queue);
}

// Dequeue an entry.  Blocks if empty
// Returns false when a SHUTDOWN entry is dequeued (signals thread to exit)
static bool queue_push(MudLogQueue* queue, const MudLogQueueEntry* entry) {
    // TODO: Implement
    //      - Lock mutex
    pthread_mutex_lock(&queue->mutex);
    //      - While count == capacity; pthread_cond_wait(&not_full, &mutex)
    while (queue->count == queue->capacity) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }

    //      - memcpy entry into entries[tail]
    memcpy(&queue->entries[queue->tail], entry, sizeof(MudLogQueueEntry));

    //      - Advance tail: tail = (tail + 1) % capacity
    queue->tail = (queue->tail + 1) % queue->capacity;

    //      - Increment count
    queue->count++;

    //      - Signal not_empty (wake consumer)
    pthread_cond_signal(&queue->not_empty);

    //      - Unlock mutex
    pthread_mutex_unlock(&queue->mutex);

    return true;
}

// Dequeue an entry.  Blocks if empty
// Returns false when a SHUTDOWN entry is dequeued (signals thread to exit)
static bool queue_pop(MudLogQueue* queue, MudLogQueueEntry* out) {
    // TODO: Implement
    //      - Lock mutex
    pthread_mutex_lock(&queue->mutex);

    //      - While count == 0: pthread_cond_wait(&not_empty, &mutex)
    while (queue->count == 0) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }

    //      - memcpy entries[head] into out
    memcpy(queue->entries, &queue->entries[queue->head], sizeof(MudLogQueueEntry));

    //      - Advance head: head = (head + 1) % capacity
    queue->head = (queue->head + 1) % queue->capacity;

    //      - Decrement count
    queue->count--;

    //      - Signal not_full (wake any blocked producers)
    pthread_cond_signal(&queue->not_full);

    //      - Unlock mutex
    pthread_mutex_unlock(&queue->mutex);

    //      - Return (out->type != MUD_LOG_MSG_SHUTDOWN)
    return (out->type != MUD_LOG_MSG_SHUTDOWN);
}

// ---- Consumer Thread -------------------------------------------------------------------------------------------------

// Dispatch a single record to a specific sink (with NULL/level checks)
static void dispatch_to_sink(MudLogSink* sink, const MudLogQueueEntry* entry) {
    // TODO: Implement
    //      - NULL check sink and sink->write
    assert(sink != NULL && "dispatch_to_sink called with NULL sink");
    assert(entry != NULL && "dispath_to_sink called with NULL entry");
    assert(sink->write != NULL && "dispatch_to_sink called with sink->write == NULL");
    if (sink == NULL || entry == NULL || sink->write == NULL) {
        fprintf(stderr, "[WARN]mud_log_thread.c: dispatch_to_sink called with NULL sink or entry\n");
        return;
    }

    //      - Check entry->level >= sink->min_level
    if (entry->level >= sink->min_level) {
        //      - Build a MudLogRecord from the entry's embedded fields
        MudLogRecord record = {
            .level = entry->level;
            .timestamp = entry->timestamp;
            .file = entry->file;
            .line = entry->line;
            .message = entry->message;
        };
        //      - Call sink->write(sink, &record)
        sink->write(sink, &record);
        return;
    }
}

// Dispatch a record to all registered sinks.
// NOTE: This needs access to the global sink list from mud_log.c
//       Design choice inside
static void dispatch_to_all_sinks(const MudLogQueueEntry* entry) {
    // TODO: Implement
    //      - Iterate g_log.sinks[0..sink_count]
    MudLogRecord* g_log {
        .level = entry->level,
        .timestamp = entry->timestamp,
        .file -> entry->file,
        .line = entry->line,
        .message = entry->message,
        .target_sink = entry->target_sink,
        .flush_done = entry->flush_done,
        .flush_mutex = entry->flush_mutex,
        .flush_cond = entry->flush_cond,
        .sink_count = entry->sink_count,
    };
    for (size_t sink_count = 0; sink_count < g_log.sink_count; sink_count++) {
        if (g_log.target_sink != NULL) {
            dispatch_to_sink(g_log.target_sink, entry);
        } else if (g_log.sink[sink_count] != NULL) {
            mud_log_thread_flush_to_sink(g_log.sink[sink_count], entry);
        } else if (g_log.sink[sink_count]->flush_mutex != NULL) {
            pthread_mutex_lock(g_log.sink[sink_count]->flush_mutex);
            *g_log.sink[sink_count]->flush_done = true;
            pthread_cond_signal(g_log.sink[sink_count]->flush_cond);
            pthread_mutex_unlock(g_log.sink[sink_count]->flush_mutex);
            
        } 
        //      - Call dispatch_to_sink for each
        //      - If sink->flush != NULL, call it
        //      - If sink->flush_mutex != NULL, lock it
        //      - Signal *entry->flush_done = true
        //      - Signal entry-flush_cond
        //      - Unlock entry->flush_mutex
        //      - Unlock mutex
        //      - Return (out->type != MUD_LOG_MSG_SHUTDOWN)
        //      - Return (out->type != MUD_LOG_MSG_SHUTDOWN)

    }
    //      - Call dispatch_to_sink for each
}

// Handle a FLUSH entry: signal the waiting producer
static void handle_flush(const MudLogQueueEntry* entry) {
    // TODO: Implement
    //      - Lock entry->flush_mutex
    pthread_mutex_lock(&entry->flush_mutex);
    //      - Signal *entry->flush_done = true
    pthread_cond_signal(&entry->flush_done, true);

    //      - Signal entry-flush_cond
    pthread_cond_signal(&entry->flush_cond);
    pthread_mutex_unlock(&entry->flush_mutex);
    //      - Unlock entry->flush_mutex
}

// The consumer thread function.
static void* log_thread_func(void* arg) {
    (void)arg;
    MudLogQueueEntry entry;

#if defined(__linux__)
    pthread_setname_np(pthread_self(), "mud_log");
#elif defined(__APPLE__)
    pthread_setname_np("mud_log");
#endif

    // TODO: Implement the loop
    //      - while (queue_pop(g_log_thread.queue, &entry));
    //          switch (entry.type):
    //              case MUD_LOG_MSG_RECORD:
    //                  if entry.target_sink != NULL:
    //                      dispatch_to_sink(entry.target_sink, &entry)
    //                  else:
    //                      dispatch_to_all_sinks(&entry)
    //                  break;
    //              case MUD_LOG_MSG_FLUSH:
    //                  handle_flush(&entry)
    //                  break
    //              case MUD_LOG_MSG_SHUTDOWN:
    //                  (queue_pop already returned false, loop exits)
    //                  break
    //      - Drain any remaining entries after shutdown signal
    //      - return NULL
    while (queue_pop(g_log_thread.queue, &entry)) {
        switch (entry.type) {
            case MUD_LOG_MSG_RECORD:
                if (entry.target_sink != NULL) {
                    dispatch_to_sink(entry.target_sink, &entry)
                } else {
                    dispatch_to_all_sinks(&entry);
                }
                break;
            case MUD_LOG_MSG_FLUSH:
                pthread_mutex_lock(&entry.flush_mutex);
                *entry.flush_done = true;
                pthread_cond_signal(entry.flush_cond);
                pthread_mutex_unlock(entry.flush_mutex);
                break;
            default:
                assert(0 && "unkown message type in log queue");
                break;
        }
        MudLogQueueEntry drain_entry;
        while (queue_try_pop(g_log.thread_queue, &drain_entry)) {
            if (drain_entry.type == MUD_LOG_MSG_RECORD) {
                if (entry.target_sink != NULL) {
                    dispatch_to_sink(entry.target_sink, &entry)
                } else {
                    dispatch_to_all_sinks(&entry);
                }
            } else if (drain_entry.type == MUD_LOG_MSG_FLUSH) {
                pthread_mutex_lock(&drain_entry.flush_mutex);
                *drain_entry.flush_done = true;
                pthread_cond_signal(drain_entry.flush_cond);
                pthread_mutex_unlock(drain_entry.flush_mutex);
            }
        }
        queue_destroy(g_log.thread_queue);
        g_log.thread_queue = NULL;
    }

}

// ---- Public API Implementation ------------------------------------------------------------------------------------------

bool mud_log_thread_init(size_t queue_capacity) {
    // TODO: Implement
    if (g_log.initialized) {
        return true;
    }

    size_t capacity = (queue_capacity == 0)
        ? MUD_LOG_QUEUE_DEFAULT_CAPACITY : queue_capacity;

    g_log_thread.queue = queue_create(capacity);
    if (g_log_thread.queue == NULL) {
        fprintf(stderr, "[ERROR]mud_log_thread.c: failed to create queue");
        return false;
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 64 * 1024);    // 64k stack

    int rc = pthread_create(&g_log_thread.thread, &attr, log_thread_func, NULL);
    pthread_attr_destroy(&attr);
    if (rc != 0) {
        queue_destroy(g_log_thread.queue);
        g_log_thread.queue = NULL;
        return false;
    }

g_log_thread.running = true;

    fprintf(stderr, "[INFO]mud_log_thread.c: log thread initialized\n");

    return true;
}

void mud_log_thread_shutdown(void) {
    // TODO: Implement
    //      - If not running, return
    if (!g_log_thread.running) {
        return;
    }

    //      - Enqueue a MUD_LOG_MSG_SHUTDOWN entry
    queue_push(g_log_thread.queue, MUD_LOG_MSG_SHUTDOWN, NULL, 0, NULL, NULL);

    //      - pthread_join the thread
    pthread_join(g_log_thread.thread, NULL);

    //      - queue_destroy
    queue_destroy(g_log_thread.queue);

    //      - Set running = false, queue = NULL
    g_log_thread.running = false;
    g_log_thread.queue = NULL;
    fprintf(stderr, "[INFO]mud_log_thread.c: log thread shutdown\n");
    return true;
}

bool mud_log_thread_enqueue(MudLogLevel level, const char* file, int line, const char* timestamp, const char* message) {
    // TODO: Implement
    //      - Build a MudLogQueueEntry on the stack
    MudLogQueueEntry entry = {
        .type = MUD_LOG_MSG_RECORD;
        .level = level;
        .timestamp = timestamp;
        .file = file;
        .line = line;
        .message = message;
        .target_sink = NULL;
        .flush_done = NULL;
        .flush_mutex = NULL;
        .flush_cond = NULL;
    };

    //      - Set type = MUD_LOG_MSG_RECORD
    entry.type = MUD_LOG_MSG_RECORD;

    //      - Copy strings safely (strncpy or mud_str_copy) into embedded buffers
    strncpy(entry.timestamp, timestamp, sizeof(entry.timestamp));
    strncpy(entry.file, file, sizeof(entry.file));
    strncpy(entry.message, message, sizeof(entry.message));
    if (line != 0) {
        snprintf(entry.line, sizeof(entry.line), "%d", line);
        entry.line[sizeof(entry.line) - 1] = '\0';
    }
    
    //      - Set target_sink = NULL (all sinks)
    entry.target_sink = NULL;

    //      - Call queue_push
    queue_push(g_log_thread.queue, entry);

    //      - Apply NULL safety: if file==NULL, copy fallback string, etc.
    if (file == NULL) {
        entry.file = MUD_LOG_FALLBACK_FILE;
    }
    if (line == 0 || line < 0 || line > INT_MAX) {
        entry.line = MUD_LOG_FALLBACK_LINE;
    }
    if (message == NULL) {
        entry.message = MUD_LOG_FALLBACK_MESSAGE;
    }
    if (timestamp == NULL) {
        strncpy(entry.timestamp, MUD_LOG_FALLBACK_TIMESTAMP, sizeof(entry.timestamp));
    }
    if (entry.file == NULL || entry.line == NULL || entry.message == NULL || entry.timestamp == NULL) {
        entry.file[sizeof(entry.file) - 1] = '\0';
        entry.line[sizeof(entry.line) - 1] = '\0';
        entry.message[sizeof(entry.message) - 1] = '\0';
        entry.timestamp[sizeof(entry.timestamp) - 1] = '\0';
    }
    if (entry.level == MUD_LOG_LEVEL_COUNT) {
        entry.level = MUD_LOG_FALLBACK_LEVEL;
    }
    return true;
}

bool mud_log_thread_enqueue_to_sink(MudLogSink* sink, MudLogLevel level, const char* file, int line,
                                    const char* timestamp, const char* message) {
    // TODO: Implement
    //      - Same as above but set target_sink = sink
    //      - If sink is NULL, return false (nowhere to send it)
}

void mud_log_thread_flush(void) {
    // TODO: Implement
    //      - Create local: pthread_mutex_t, pthread_cond_t, bool done = false
    pthread_mutex_t flush_mutex;
    pthread_cond_t flush_cond;
    bool flush_done = false;
    pthread_mutex_init(&flush_mutex, NULL);
    pthread_cond_init(&flush_cond, NULL);

    //      - Build a FLUSH entry pointing to these locals
    MudLogQueueEntry flush_entry = {
        .type = MUD_LOG_MSG_FLUSH;
        .flush_mutex = &flush_mutex;
        .flush_cond = &flush_cond;
        .flush_done = &flush_done;
    };

    //      - Enqueue it
    queue_push(g_log_thread.queue, flush_entry);

    //      - Lock mutex, while (!done) cond_wait, unlock
    pthread_mutex_lock(&flush_mutex);
    while (!flush_done) {
        pthread_cond_wait(&flush_cond, &flush_mutex);
    }
    pthread_mutex_unlock(&flush_mutex);

    //      - Destroy local mutex and condvar
    pthread_mutex_destroy(&flush_mutex);
    pthread_cond_destroy(&flush_cond);
}

bool mud_log_thread_is_running(void) {
    return g_log_thread.running;
}
