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


int main()
{
    stdio_init_all();

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
