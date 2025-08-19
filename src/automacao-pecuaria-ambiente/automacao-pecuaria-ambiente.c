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

// --- FUNÇÕES DE SIMULAÇÃO E CONTROLE ---
void simular_temperatura_umidade_sensor() {
    temperatura_sensor = 20.0 + (rand() % 150) / 10.0;
    umidade_sensor = 30.0 + (rand() % 600) / 10.0;
    printf("Novos dados simulados: Temp=%.1f C, Umid=%.1f %%\n", temperatura_sensor, umidade_sensor);
}

void simular_luminosidade_sensor() {
    luminosidade_sensor = rand() % 101;
    printf("Nova luminosidade simulada: %.1f %%\n", luminosidade_sensor);
}

void acionar_rele_luz(bool ligar) {
    if (ligar) {
        if (!gpio_get(RELAY_LIGHTS_PIN)) printf("Luzes ligadas (luminosidade baixa).\n");
        gpio_put(RELAY_LIGHTS_PIN, 1);
    } else {
        if (gpio_get(RELAY_LIGHTS_PIN)) printf("Luzes desligadas (luminosidade alta).\n");
        gpio_put(RELAY_LIGHTS_PIN, 0);
    }
}

void acionar_rele_ventilador(float temperatura) {
    if (temperatura > TEMPERATURE_FAN_THRESHOLD) {
        if (!gpio_get(RELAY_FAN_PIN)) printf("Ventilador ligado (temperatura alta: %.1f C).\n", temperatura);
        gpio_put(RELAY_FAN_PIN, 1);
    } else {
        if (gpio_get(RELAY_FAN_PIN)) printf("Ventilador desligado (temperatura OK: %.1f C).\n", temperatura);
        gpio_put(RELAY_FAN_PIN, 0);
    }
}

void acionar_rele_umidificador(float umidade) {
    if (umidade < HUMIDITY_HUMIDIFIER_THRESHOLD) {
        if (!gpio_get(RELAY_HUMIDIFIER_PIN)) printf("Umidificador ligado (umidade baixa: %.1f %%).\n", umidade);
        gpio_put(RELAY_HUMIDIFIER_PIN, 1);
    } else {
        if (gpio_get(RELAY_HUMIDIFIER_PIN)) printf("Umidificador desligado (umidade OK: %.1f %%).\n", umidade);
        gpio_put(RELAY_HUMIDIFIER_PIN, 0);
    }
}

void salvar_historico_sensores() {
    datetime_t t;
    rtc_get_datetime(&t);
    for (int i = MAX_HISTORICO - 1; i > 0; i--) {
        historico_sensores[i] = historico_sensores[i - 1];
        historico_valido[i] = historico_valido[i - 1];
    }
    historico_sensores[0] = (historico_t){.temperatura = temperatura_sensor, .umidade = umidade_sensor, .timestamp = t};
    historico_valido[0] = true;
    printf("Novo registro salvo: %02d/%02d %02d:%02d:%02d - Temp=%.1f, Umid=%.1f\n",
           t.day, t.month, t.hour, t.min, t.sec, temperatura_sensor, umidade_sensor);
}

// --- FUNÇÕES DE INTERFACE (DISPLAY E WEB) ---
void atualizar_display_oled() {
    char text[32];
    memset(oled_buffer, 0, sizeof(oled_buffer));

    sprintf(text, "Temp: %.1f C", temperatura_sensor);
    ssd1306_draw_string(oled_buffer, 0, 0, text);

    sprintf(text, "Umid: %.1f %%", umidade_sensor);
    ssd1306_draw_string(oled_buffer, 0, 10, text);

    sprintf(text, "Luz:    %s", gpio_get(RELAY_LIGHTS_PIN) ? "Ligada" : "Desligada");
    ssd1306_draw_string(oled_buffer, 0, 20, text);
    
    sprintf(text, "Vent:   %s", gpio_get(RELAY_FAN_PIN) ? "Ligado" : "Desligado");
    ssd1306_draw_string(oled_buffer, 0, 30, text);

    sprintf(text, "Umidif: %s", gpio_get(RELAY_HUMIDIFIER_PIN) ? "Ligado" : "Desligado");
    ssd1306_draw_string(oled_buffer, 0, 40, text);

    sprintf(text, "%s", (cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_UP) ? "WiFi: Conectado" : "WiFi: Desconectado");
    ssd1306_draw_string(oled_buffer, 0, 54, text);

    render_on_display(oled_buffer, &frame_area);
}

void create_http_response() {
    char history_table_rows[2048] = "";
    char *ptr = history_table_rows;
    for (int i = 0; i < MAX_HISTORICO; i++) {
        if (historico_valido[i]) {
            ptr += sprintf(ptr, "<tr><td>%02d/%02d/%04d %02d:%02d:%02d</td><td>%.1f &deg;C</td><td>%.1f %%</td></tr>",
                           historico_sensores[i].timestamp.day, historico_sensores[i].timestamp.month, historico_sensores[i].timestamp.year,
                           historico_sensores[i].timestamp.hour, historico_sensores[i].timestamp.min, historico_sensores[i].timestamp.sec,
                           historico_sensores[i].temperatura, historico_sensores[i].umidade);
        }
    }

    const char *light_status = gpio_get(RELAY_LIGHTS_PIN) ? "Ligadas" : "Desligadas";
    const char *fan_status = gpio_get(RELAY_FAN_PIN) ? "Ligado" : "Desligado";
    const char *humidifier_status = gpio_get(RELAY_HUMIDIFIER_PIN) ? "Ligado" : "Desligado";

    snprintf(http_response, sizeof(http_response),
             "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
             "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>Pico W Home Control</title><meta http-equiv=\"refresh\" content=\"10\">"
             "<style>body{font-family:sans-serif;background:#f4f4f4;color:#333;}.container{max-width:800px;margin:auto;padding:20px;background:#fff;"
             "border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}table{width:100%%;border-collapse:collapse;margin-bottom:20px;}th,td{padding:12px;"
             "text-align:left;border-bottom:1px solid #ddd;}th{background-color:#007bff;color:white;}h1,h2{color:#007bff;}a.button{display:inline-block;"
             "padding:10px 15px;background-color:#28a745;color:white;text-decoration:none;border-radius:5px;}</style></head><body><div class=\"container\">"
             "<h1>Painel de Controle - Pico W</h1><h2>Status Atual</h2><table><tr><th>Sensor/Atuador</th><th>Valor/Estado</th></tr>"
             "<tr><td>Temperatura</td><td>%.1f &deg;C</td></tr><tr><td>Umidade</td><td>%.1f %%</td></tr><tr><td>Luminosidade</td><td>%.1f %%</td></tr>"
             "<tr><td>Luzes</td><td>%s</td></tr><tr><td>Ventilador</td><td>%s</td></tr><tr><td>Umidificador</td><td>%s</td></tr></table>"
             "<h2>Histórico Recente dos Sensores</h2><p><a href=\"/download\" class=\"button\">Baixar Histórico (CSV)</a></p>"
             "<table><tr><th>Data e Hora</th><th>Temperatura</th><th>Umidade</th></tr>%s</table></div></body></html>\r\n",
             temperatura_sensor, umidade_sensor, luminosidade_sensor, light_status, fan_status, humidifier_status, history_table_rows);
}

void create_csv_content() {
    char *ptr = csv_content;
    *ptr = '\0';
    datetime_t t;
    rtc_get_datetime(&t);
    ptr += sprintf(ptr, "# Relatório de Histórico dos Sensores - Pico W\n");
    ptr += sprintf(ptr, "# Gerado em: %04d-%02d-%02d %02d:%02d:%02d\n\n", t.year, t.month, t.day, t.hour, t.min, t.sec);
    ptr += sprintf(ptr, "Timestamp;Temperatura (C);Umidade (%%)\n");
    for (int i = 0; i < MAX_HISTORICO; i++) {
        if (historico_valido[i]) {
            ptr += sprintf(ptr, "%04d-%02d-%02d %02d:%02d:%02d;%.1f;%.1f\n",
                           historico_sensores[i].timestamp.year, historico_sensores[i].timestamp.month, historico_sensores[i].timestamp.day,
                           historico_sensores[i].timestamp.hour, historico_sensores[i].timestamp.min, historico_sensores[i].timestamp.sec,
                           historico_sensores[i].temperatura, historico_sensores[i].umidade);
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
