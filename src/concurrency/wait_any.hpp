#ifndef __CONCURRENCY_WAIT_ANY_HPP__
#define __CONCURRENCY_WAIT_ANY_HPP__

class signal_t;

/* Waits for up to five signals at the same time. Returns when any one of them
is pulsed. */

void wait_any_lazily_unordered(signal_t *s1);
void wait_any_lazily_unordered(signal_t *s1, signal_t *s2);
void wait_any_lazily_unordered(signal_t *s1, signal_t *s2, signal_t *s3);
void wait_any_lazily_unordered(signal_t *s1, signal_t *s2, signal_t *s3, signal_t *s4);
void wait_any_lazily_unordered(signal_t *s1, signal_t *s2, signal_t *s3, signal_t *s4, signal_t *s5);

#endif /* __CONCURRENCY_WAIT_ANY_HPP__ */
