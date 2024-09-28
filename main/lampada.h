#ifndef LAMPADA_H
#define LAMPADA_H

#include "driver/gpio.h"



// Funções para inicialização e controle dos relés
void relay_init(void);
void relay_set_state(int relay, int state);

#endif // LAMPADA_H
