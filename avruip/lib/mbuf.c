#include "mbuf.h"

#define POOL_SIZE	128

static struct mbuf mbuf_pool[POOL_SIZE];

static struct mbuf *free_list;

void mbuf_init(void)
{
  uint8_t i;

  for (i = 0; i < POOL_SIZE; i++)
  {
    struct mbuf *m = &mbuf_pool[i];
    mbuf_free(m);
  }
}

struct mbuf *mbuf_alloc(void) __attribute__ ((noinline));

struct mbuf *mbuf_alloc(void)
{
  struct mbuf *m = free_list;
  if (m)
  {
    struct mbuf *next = m->next;
    if (next)
      next->prev = NULL;
    free_list = next;
    m->next = NULL;
  }
  return m;
}

void mbuf_free(struct mbuf *m) __attribute__ ((noinline));

void mbuf_free(struct mbuf *m)
{
  m->next = free_list;
  m->prev = NULL;
  if (free_list)
    free_list->prev = m;
  m->length = 0;
  free_list = m;
}

void mbuf_free_chain(struct mbuf *m)
{
  while (m)
  {
    struct mbuf *next = m->next;
    mbuf_free (m);
    m = next;
  }
}

struct mbuf *copy_to_mbufs(uint8_t *buffer, uint16_t length)
{
  struct mbuf *first = NULL, *last = NULL;
  while (length)
  {
    struct mbuf *m = mbuf_alloc ();
    if (m == NULL)
    {
      // Out of memory
      mbuf_free_chain (first);
      return NULL;
    }
    if (first == NULL)
    {
      first = m;
    }
    else
    {
      m->prev = last;
      last->next = m;
    }
    last = m;
    uint8_t to_copy = MBUF_SIZE;
    if (to_copy > length)
      to_copy = length;
    memcpy (m->data, buffer, to_copy);
    m->length = to_copy;
    buffer += to_copy;
    length -= to_copy;
  }
  return first;
}
