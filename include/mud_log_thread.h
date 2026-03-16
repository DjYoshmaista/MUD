#ifndef MUD_LOG_THREAD_H
#define MUD_LOG_THREAD_H

#include "mud_log_types.h" 
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/*      Queue entry message types
        RECORD:     Normal log message to be dispatched
        FLUSH:      Synchronization barrier - consumer signals when reached
        SHUTDOWN:   Consumer should drain remaining entries and exit

        Future: Update UI (UI_UPDATE), Progress Trackin (PROGRESS) ...
*/

typedef enum MudLogMsgType {
    MUD_LOG_MSG_RECORD,
    MUD_LOG_MSG_FLUSH,
    MUD_LOG_MSG_SHUTDOWN,
} MudLogMsgType;

/*      A single queue entry -- Lives in ring buffer
        Design Note: All string data is embedded (not pointed to) b/c pointer's stack frame will be gone by the time the consumer reads this.  sizeof(MudLogQueueEntry) = 4.3kb
        Ring buffer of 256 entries = 1.1MB
*/

#ifndef MUD_LOG_QUEUE_MSG_MAX
#define MUD_LOG_QUEUE_MSG_MAX 4096
#endif

#ifndef MUD_LOG_QUEUE_FILE_MAX
#define MUD_LOG_QUEUE_FILE_MAX 128
#endif

#ifndef MUD_LOG_QUEUE_TIMESTAMP_MAX
#define MUD_LOG_QUEUE_TIMESTAMP_MAX 32
#endif

typedef struct MudLogQueueEntry {
    MudLogMsgType type;
    
    // Log record data (only meaningful when type = MUD_LOG_MSG_RECORD)
    MudLogLevel level;
    int line;
    char timestamp[MUD_LOG_QUEUE_TIMESTAMP_MAX];
    char file[MUD_LOG_QUEUE_FILE_MAX];
    char message[MUD_LOG_QUEUE_MSG_MAX];

    // Sink targeting: NULL means dispatch to all sinks
    MudLogSink* target_sink;

    // Flush synchronization (only meaninguful when type == MUD_LOG_MSG_FLUSH)
    // Producer sets these, waits on the condvar
    // Consumer signals when it processes the FLUSH entry
    // These are pointers to producer owned stack variables
    pthread_mutex_t*    flush_mutex;
    pthread_cond_t*     flush_cond;
    bool*               flush_done;
} MudLogQueueEntry;

#ifndef MUD_LOG_QUEUE_DEFAULT_CAPACITY
#define MUD_LOG_QUEUE_DEFAULT_CAPACITY 256
#endif

/*  Initialize the logging thread subsystem
    Creates the queue and spawns the consumer thread
    Call AFTER mud_log_init() [sinks must be registered first]
    @param queue_capacity   Number of entries in the ring buffer (0 = default 256)
    @return true on success, false on failure
*/
bool mud_log_thread_init(size_t queue_capacity);

/*  Shutdown the logging thread
    Enqueues a SHUTDOWN message, waits for the thread to drain and exit, then destroys the queue
    Call BEFORE mud_log_shutdown() [sinks must still be valid]
*/
void mud_log_thread_shutdown(void);

/* Forward declarations */
typedef struct MudLogQueue MudLogQueue;

/*  Enqueue a log record for async dispatch to all sinks

    Called by mud_log_writev() instead of dispatching directly.
    Blocks if queue is full (backpressure)

    @return true if enqueued, false on failure
*/
bool mud_log_thread_enqueue(MudLogLevel level, const char* file, int line, const char* timestamp, const char* message);

/*  Enqueue a log record targeted at a specific sink
    @return true if enqueued, false on failure
*/
bool mud_log_thread_enqueue_to_sink(MudLogSink* sink, MudLogLevel level, const char* file, int line,
                                    const char* timestamp, const char* message);

/*  Flush: block until all currently-enqueued messages are processed

    Useful for tests and shutdown sequences where you need to guarantee output is visible before proceeding
*/
void mud_log_thread_flush(void);

//  Query whether the logging thread is running
bool mud_log_thread_is_running(void);

#ifdef __cplusplus
}
#endif

#endif // MUD_LOG_THREAD_H
