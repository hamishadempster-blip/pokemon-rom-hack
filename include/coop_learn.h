#ifndef GUARD_COOP_LEARN_H
#define GUARD_COOP_LEARN_H

// "Who can learn this?" (src/coop_learn.c): the bag's CAN LEARN on a TM/HM.
void CoopLearn_Begin(u16 move);
void CoopLearn_Summary(u8 *dest);
bool8 CoopLearn_NextPage(u8 *dest);

#endif // GUARD_COOP_LEARN_H
