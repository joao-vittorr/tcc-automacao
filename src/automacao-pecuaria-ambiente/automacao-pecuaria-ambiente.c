/**
 * @file main.c
 * @author João Vitor Alexandre Ribeiro 
 * @brief Projeto de Automação com Raspberry Pi Pico W
 * @version 4.0
 * @date 2025-07-01
 * * @details
 * Este projeto implementa um sistema de automação que monitora e controla
 * um ambiente usando sensores (simulados) e atuadores (relés).
 * * Funcionalidades:
 * - Monitora temperatura, umidade e luminosidade.
 * - Controla luzes, um ventilador e um umidificador via relés.
 * - Exibe o status em um display OLED SSD1306.
 * - Fornece um painel de controle web (dashboard) com atualização automática.
 * - Permite o download do histórico de sensores em formato CSV.
 * - Usa uma matriz de LEDs 5x5 (Neopixel) como indicador visual do estado dos atuadores.
 * - Conecta-se à rede Wi-Fi com lógica de reconexão automática.
 */

// --- BIBLIOTECAS (INCLUDES) ---
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/clocks.h"
#include "hardware/rtc.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "pico/util/datetime.h"
#include "ws2818b.pio.h"

// --- DEFINIÇÕES E CONSTANTES GLOBAIS ---
// I2C para Display OLED
const uint I2C_SDA = 14;
const uint I2C_SCL = 15;
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64

// Matriz de LEDs Neopixel (WS2812B)
#define LED_COUNT 25
#define LED_PIN_PIO 7

// Credenciais Wi-Fi
#define WIFI_SSID "S23"
#define WIFI_PASS "#Vitor123@"

// Pinos dos Relés
#define RELAY_LIGHTS_PIN 26
#define RELAY_FAN_PIN 27
#define RELAY_HUMIDIFIER_PIN 28

// Limiares de acionamento
#define LUMINOSITY_THRESHOLD 40.0         // Acima deste valor, a luz apaga
#define TEMPERATURE_FAN_THRESHOLD 28.0    // Acima deste valor, o ventilador liga
#define HUMIDITY_HUMIDIFIER_THRESHOLD 45.0// Abaixo deste valor, o umidificador liga

// Configuração do histórico
#define MAX_HISTORICO 10
const uint32_t SENSOR_READ_INTERVAL_MS = 5 * 60 * 1000; // 5 minutos

// --- ESTRUTURAS E VARIÁVEIS GLOBAIS ---
// Sensores (simulados)
float temperatura_sensor = 25.0;
float umidade_sensor = 50.0;
float luminosidade_sensor = 50.0;

// Estrutura para o histórico
typedef struct {
    float temperatura;
    float umidade;
    datetime_t timestamp;
} historico_t;

historico_t historico_sensores[MAX_HISTORICO];
bool historico_valido[MAX_HISTORICO];

// Buffers para Webserver e OLED
char http_response[4096];
char csv_content[2048];
uint8_t oled_buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
struct render_area frame_area;

// Estruturas para LEDs Neopixel
struct pixel_t {
    uint8_t G, R, B;
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t;

npLED_t leds[LED_COUNT];
PIO np_pio;
uint sm;

// --- FUNÇÕES PARA LEDS NEOPIXEL ---
void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0) {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true);
    }
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);
    for (uint i = 0; i < LED_COUNT; ++i) leds[i] = (npLED_t){0, 0, 0};
}

void npWrite() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100);
}

// Cores pré-definidas para os indicadores
const pixel_t COR_DESLIGADO = {0, 0, 0};
const pixel_t COR_LUZ = {30, 100, 0};       // Amarelo (G, R, B)
const pixel_t COR_VENTILADOR = {50, 50, 50}; // Branco
const pixel_t COR_UMIDIFICADOR = {50, 0, 50};  // Ciano/Azul

// Função para atualizar a matriz de LEDs com base no estado dos relés
// Lógica usa LINHAS como indicadores.
void atualizar_matriz_leds() {
    bool luz_ligada = gpio_get(RELAY_LIGHTS_PIN);
    bool ventilador_ligado = gpio_get(RELAY_FAN_PIN);
    bool umidificador_ligado = gpio_get(RELAY_HUMIDIFIER_PIN);

    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            int led_index = y * 5 + x;
            
            if (y == 0) { // Linha 1 (1x5) para o Ventilador
                leds[led_index] = ventilador_ligado ? COR_VENTILADOR : COR_DESLIGADO;
            } else if (y == 1) { // Linha 2 (1x5) para o Umidificador
                leds[led_index] = umidificador_ligado ? COR_UMIDIFICADOR : COR_DESLIGADO;
            } else if (y == 2) { // Linha 3 (1x5) para as Luzes
                leds[led_index] = luz_ligada ? COR_LUZ : COR_DESLIGADO;
            } else { // Linhas restantes (y=3 e y=4) ficam desligadas
                leds[led_index] = COR_DESLIGADO;
            }
        }
    }
}


int main()
{
    stdio_init_all();

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
