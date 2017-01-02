/** @brief low-level FIFO templace using a circular buffer
 * @author JD <jdbrossillon@fly-n-sense.com>
 */

#ifndef FIFO_H
#define FIFO_H

#ifndef T
#error "T is not defined, define T to your template type"
#endif

#include <inttypes.h>
typedef uint8_t bool_t;

typedef struct {
  T *head;
  T *tail;
  T *data;
  uint16_t data_len;
}fifo_t;

/** @brief Pointer to the end of FIFO */
#define FIFO_END(fifo) ((fifo)->data + (fifo)->data_len)

/** @brief Initialize FIFO with buffer
 * @param len number of element which can be stored in buffer
 * @warning : FIFO can hold size-1 elements */
static inline void fifo_init(fifo_t *fifo, T *buffer, uint16_t len) {
  // initialize buffer
  fifo->data = buffer;
  fifo->data_len = len;
  // set head and tail to start of data buffer
  fifo->head = fifo->tail = fifo->data;
}

/** @brief Clear FIFO, size will be 0 */
static inline void fifo_clear(fifo_t *fifo) {
  // set head and tail to start of data buffer
  fifo->head = fifo->tail = fifo->data;
}

/** @brief Return avaible free space in FIFO */
static inline uint16_t fifo_freespace(fifo_t *fifo) {
  int32_t delta = fifo->data_len - 1 - (fifo->tail - fifo->head);
  if(delta >= fifo->data_len)
    delta -= fifo->data_len;
  return delta;
}

/** @brief Return stored elements in FIFO */
static inline uint16_t fifo_size(fifo_t *fifo) {
  uint16_t fs = fifo_freespace(fifo);
  return fifo->data_len - 1 - fs;
}

/** @brief Return TRUE if FIFO is full */
static inline bool_t fifo_isfull(fifo_t *fifo) {
  // check if tail+1 equals :
  // - end of buffer if head is located at the start of buffer
  // - head of buffer otherwise
  if(fifo->head == fifo->data)
    return fifo->tail+1 == FIFO_END(fifo);
  else
    return fifo->tail+1  == fifo->head;
}

/** @brief Return TRUE if FIFO is empty */
static inline bool_t fifo_isempty(fifo_t *fifo) {
  return fifo->tail == fifo->head;
}

/**@brief Push c to FIFO */
static inline void fifo_push(fifo_t *fifo, T c) {
  *fifo->tail = c;
  if(++(fifo->tail) == FIFO_END(fifo)) {
    fifo->tail = fifo->data;
  }
}

static inline T fifo_at(fifo_t *fifo, uint16_t index) {
  T *c = fifo->head;
  uint16_t i;
  for(i = 0; i < index; i++)
  {
    c++;
    if(c == FIFO_END(fifo)) {
      c = fifo->data;
    }
  }
  return *c;
  
  
}

/**@brief Pop from FIFO */
static inline T fifo_pop(fifo_t *fifo) {
  T c = *fifo->head;
  if(++(fifo->head) == FIFO_END(fifo)) {
    fifo->head = fifo->data;
  }
  return c;
}

/**@brief Get FIFO tail */
static inline T fifo_tail(fifo_t *fifo) {
  return *fifo->tail;
}

/**@brief Get FIFO head, do not pop it */
static inline T fifo_head(fifo_t *fifo) {
  return *fifo->head;
}

#endif//FIFO_H
