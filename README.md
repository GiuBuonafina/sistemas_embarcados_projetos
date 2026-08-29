# Sistemas Embarcados - UPE

Repositório com os projetos práticos desenvolvidos na disciplina de **Sistemas Embarcados**, do curso de Engenharia da computação da **Universidade de Pernambuco (UPE)**.

## Equipe

- Giulia Buonafina
- Maria Luana Rodrigues

## Ferramentas e hardware utilizados

- **Placa:** NUCLEO-L476RG (STM32L476RG)
- **IDE:** STM32CubeIDE
- **Biblioteca:** STM32 HAL (Hardware Abstraction Layer)

## Projetos

| Pasta | Projeto | Descrição |
|---|---|---|
| [`projeto_1/`](./projeto_1) | GPIO e PWM de Software | Controle da saída GPIO do LED da placa com um PWM implementado por software (liga/desliga cronometrado por HAL_Delay), com ajuste de frequência e duty cycle. |

Cada pasta de projeto contém seu próprio `README.md` com o objetivo específico, o hardware/pinos utilizados, o código-fonte relevante e os resultados observados.

## Estrutura de cada projeto

Cada projeto é um projeto completo do STM32CubeIDE, contendo:

- `Core/` — código-fonte da aplicação (`main.c`, headers, etc.)
- `Drivers/` — bibliotecas HAL/CMSIS geradas pela ferramenta
- `*.ioc` — arquivo de configuração do STM32CubeMX com os pinos e periféricos utilizados

## Como abrir um projeto

1. Clone este repositório.
2. Abra o STM32CubeIDE.
3. Vá em **File → Open Projects from File System** e selecione a pasta do projeto desejado (ex.: `projeto_1`).
4. Compile (**Build**) e, com a placa NUCLEO-L476RG conectada via USB, grave o código (**Run**).
