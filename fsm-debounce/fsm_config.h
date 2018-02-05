#define NUM_STATES 7
#define NUM_EVENTS 9

// definicao dos possiveis eventos gerados pelos estados da FSM
typedef enum event_ {wifi_not_connected, wifi_connected, server_not_connected, server_connected, coleta_btn, coleta_time, goto_led, goto_start, repeat} event;
// definicao das funcoes que implementam o comportamento de cada estado

event wifi_check_state(void);
event server_check_state(void);
event start_state(void);
event coleta_state(void);
event coleta_btn_state(void);
event empty_state(void);
event led_on_state(void);

// array de ponteiros para as funcoes dos estados
event (* state_functions[])(void) = {wifi_check_state, server_check_state, start_state, coleta_btn_state, coleta_state, led_on_state, empty_state};
// definicao dos nomes dos estados
typedef enum state_ {wifi_check, server_check, start, coletaBtn, coleta, led, empty} state;
// estrutura que define as transicoes dos estados
state state_transitions[NUM_STATES][NUM_EVENTS] = {{wifi_check, server_check, empty, empty, empty, empty, start, empty, wifi_check}, //wifi check
                                                   {empty, empty, wifi_check, start, empty, empty, empty, start, server_check}, //server check
                                                   {empty, empty, empty, empty, coletaBtn, coleta, empty, start, start}, //start
                                                   {empty,empty,empty,empty, empty, empty, led, start, coletaBtn}, // coleta btn
                                                   {empty,empty,empty,empty, empty, empty, led, start, coleta}, // coleta
                                                   {empty,empty,empty,empty, empty, empty, empty, start, led}, // led
                                                   {empty,empty,empty,empty, empty, empty, empty, empty, empty}}; // empty
// definicao dos estados inicial e final
#define EXIT_STATE empty
#define ENTRY_STATE wifi_check

// funcao que implementa a ransicao de estados
state lookup_transitions(state cur_state, event cur_evt) {
  return state_transitions[cur_state][cur_evt];
}

