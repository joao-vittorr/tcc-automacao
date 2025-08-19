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

#include <stdio.h>
#include "pico/stdlib.h"



int main()
{
    stdio_init_all();

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
