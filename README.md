# Trabalho Final (2024-1)

Trabalho Final da disciplina de Fundamentos de Sistemas Embarcados - Automação Residencial

## Alunos

| Matrícula | Aluno                                                            |
| --------- | -----------------------------------------------------------------|
| 190030291	| [Jackes da Fonseca](https://github.com/jackesfonseca)            |
| 180016491 | [Fernando Vargas](https://github.com/SFernandoS)            |
| 180136925 | [Hugo Moura](https://github.com/hugorochaffs)      |
| 180130722 | [Samuel Bacelar](https://github.com/SamuelNoB) |

## Visão Geral

O sistema foi construído com o uso de uma placa ESP32, que integra um conjunto de sensores de entrada e saída para a detecção e interação com o usuário. A implementação das diversas funcionalidades foi realizada por meio de códigos em linguagem C

| Microcontrolador  | Versão | Framework |  Linguagem  |
| :---------------: | ------ | --------- | ----------- |
|      Esp32        | 5.3  | Espressif |      C      |

## DASHBOARD AUTO_HOME
[DASHBOARD](http://thingsboard.lappis.rocks:443/dashboards/5e79e060-60fe-11ef-a0ba-5ffd78de135f)

## Miro
Planejamento incial realizado pela equipe
[Miro](https://miro.com/welcomeonboard/NEhoaGI2OUp4d2hjWlF5ejlrb3hNTmdDTkYyTXA4Q3F4N3VGYVFkdzR3NGg0RHFoVGhpc2p3Z2JaWUdSTnN4SnwzMDc0NDU3MzU0ODM5NDcyMTU0fDI=?share_link_id=266227491757)

## SENSORES/ATUADORES
Lista dos sensores e componentes
- DHT11 (PLACA TEMP-HUMID)
- GAS (PLACA GAS/CHAMAS)
- CHAMAS (PLACA GAS/CHAMAS)
- LDR com resistor 10K ohms (PLACA OLED)
- OLED (PLACA OLED)
- BUZZER (PLACA OLED)
- PLACA COM 2 RELES (PLACA RELES)
- LED DA ESP (PWM - PLACA OLED) - **ANALÓGICO**
- BOTOES DA ESP (PLACA RELE, PLACA OLED)

### Funcionalides implementadas

Foram implementados diversos recursos, como detecção de fogo, vazamento de gás, temperatura e umidade ambiente, controle de lâmpada por luminosidade de forma remota, entre outros.

**Todas as placas compartilham um único firmware, podendo ser totalmente configurada via menuconfig**

**As placas possuem modo de fácil configuração de wifi (WIFI MANAGER)**

- **Operações Básicas:**
  - Ativar/Desativar alarme
  - Parar o alarme por botão ou pelo dashboard quando disparado
  - Ligar e desligar a alimentação geral da casa
  - Ligar e desligar lâmpada via botão ou via dashboard
    
- **PLACA OLED - Hugo Rocha:**
  - Tela com informações de estado do alarme, temperatura, umidade, luminosidade, estado da lampada e estado da alimentação geral, além de quando o alarme está disparado aparecem as seguintes mensagens em tela inteira: GAS, FOGO ou GAS/FOGO
  - Ativação/Desativação do alarme por meio do botão presente na esp32
  - Gravação/Leitura de informações na memória não volátil (NVS), sendo elas:
    - Estado do alarme entre ligado e desligado
    - Estado do alarme entre disparado ou não
    - Caso disparado, por qual tipo de sensor
  - Buzzer para a sinalização sonora de disparo
  - Led em PWM oscilando para sinalização visual do disparo
  - Sensor analógico de luminosidade (LDR) com valores em porcentagem e luminância (LUX)
  - Monitoramento em tempo real no dashboard dos seguintes itens:
    - Estado do alarme, ligado ou desligado
    - Estado do alarme, disparado ou não
    - Estado do Buzzer, tocando ou não
    - Estado do led PWM oscilando ou desligado
    - Luminosidade em lux
    - Luminosidade em porcentagem
  - Controle via dashboard dos seguintes itens:
    - Liga/Desliga alarme
    - Para o alarme se disparado
  - Envio de comando para ligar ou desligar lâmpada com base na luminosidade

![WhatsApp Image 2024-09-05 at 14 33 33](https://github.com/user-attachments/assets/83b5efec-875d-48f2-a20e-fe75edfb4d01)

- **PLACA SENSORES - Fernando Vargas**
  - Sensor de temperatura e envio para o dashboard e outras placas
  - Sensor de umidade e envio para o dashboard e outras placas
  - Sensor analógico de luminosidade (LDR) com valores em porcentagem e luminância (LUX)
  - Dashboard com gráfico de temperatura, umidade e luminosidade

![WhatsApp Image 2024-09-05 at 14 24 22](https://github.com/user-attachments/assets/d8d1cc75-3777-4e91-ba34-41b2925b5f9e)


- **PLACA RELES - Jackes Fonseca**
  - Ativação e desativação de lâmpada por comando recebido por outra placa ou pelo botão físico dessa placa
  - Ativação e desativação da alimentação geral utilizando comunicação com o thingsboard
  - Envio de status dos relés para o dashboard e outras placas em tempo real
  - Leitura/Gravação do estado dos relés em memória NVS, voltando ao estado em que estava ao reiniciar
  - Visualização do status dos relés (lampada e alimentação geral) no dashbard

![WhatsApp Image 2024-09-05 at 14 25 43](https://github.com/user-attachments/assets/7be44960-d50e-4307-a193-d24bb43b5db2)


- **PLACA SENSORES GAS E FOGO - Samuel Bacelar**
  - Atua em modo LOWPOWER (Deep-Sleep), com acionamento através dos sensores
  - Possui sensor de vazamento de gás
  - Possui sensor de fogo
  - Aproveita do mesmo firmware de todas as outras placas, sendo definido o LOWPOWER por menuconfig
  - Monitoramento de acionamento por meio do dashbord

![WhatsApp Image 2024-09-05 at 14 34 02](https://github.com/user-attachments/assets/bbe57ce9-f4b9-4a5e-88c2-9bc7e19afcc5)


### Interface - [Dashboard](http://thingsboard.lappis.rocks:443/dashboards/5e79e060-60fe-11ef-a0ba-5ffd78de135f):


### 🔂 Executando o Projeto

#### Clone o repositório

```bash 
$ git clone git@github.com:FGA-FSE/trabalho-final-auto-home.git
```

Observação: Caso não possua o git instalado será nessario a instalação. Sigua o [Tutorial](https://git-scm.com/book/pt-br/v2/Começando-Instalando-o-Git).

#### Instale a extenção da Esp no VSCode

#### Siga o tutorial de instalação da ESP no seu computador

[Windows](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/windows-setup.html)
[Linux e MacOs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/linux-macos-setup.html)

#### Verifique se a placa está limpa para rodar o projeto

```bash 
$ idf.py fullclean
```

#### Construa o projeto (build)

```bash 
$ idf.py build
```

#### Entre no menuconfig e faça a seguinte configuração:
* wi-fi
  - Habilite configuração manual
  - Insira o SSID e senha
* MQTT thingsboard
  - Adicione o token referente a placa que se deseja controlar
* MQTT entre placas
  - Adicione o broker: broker.hivemq.com

#### Faça novamente a build

#### Compile e rode a placa

```bash
$ idf.py flash monitor
```

Ou execute os seguintes comandos na extensão do VSCode:
- FullClean:

- Build / Flash / Monitor:

Obs: Caso esteja utilizando Windows, entre no Gerenciador de Dispotivos para descobrir qual porta (COM) está conectada sua placa. Após isso, selecione a porta correta no VSCode clicando em:

Selecionando a porta indicada.

Obs2: Caso esteja utilizando Windows, na hora que for dar o flash, selecione a opção UART

