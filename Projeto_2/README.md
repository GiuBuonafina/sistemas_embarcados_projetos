# Projeto 2 — Controle da cadência de piscar de um LED com EXTI

Disciplina de **Sistemas Embarcados** — Universidade de Pernambuco (UPE)

**Equipe:** Giulia Buonafina, Maria Luana Rodrigues

## Objetivo

Desenvolver um sistema, utilizando a placa NUCLEO-L476RG e o periférico **EXTI** (*External Interrupt*), em que um LED pisque com frequência variável, controlada pelo botão de usuário (botão azul).

## Resultado

O LED inicia piscando a 1 Hz. A cada acionamento do botão azul, a cadência alterna corretamente entre 1 Hz e 2 Hz, conforme especificado, através da interrupção EXTI associada ao pino PC13.