int ldr = A0;
int potenciometro = A1;
int ledVerde = 8;
int ledVermelho = 9;
int buzzer = 10;
int botaoConfirma = 7;

int pontos = 0;
int diasConsecutivos = 0;

unsigned long horarioRemedio = 0;
unsigned long horarioAtual = 0;
int horarioConfigurado = 0;

int leituraLDR[5];
int posLeitura = 0;

int statusTomou = 0;
int alarmeDisparado = 0;
int mensagemAlarme = 0;

int loopAtivo = 0;
int totalDoses = 0;
int dosesRestantes = 0;
unsigned long intervaloDoses = 0;

int etapaConfiguracao = 0;
unsigned long tempoPendente = 0;

unsigned long tempoInicioAlarme = 0;

void setup() {
    pinMode(ledVerde, OUTPUT);
    pinMode(ledVermelho, OUTPUT);
    pinMode(buzzer, OUTPUT);
    pinMode(botaoConfirma, INPUT_PULLUP);

    Serial.begin(9600);
    Serial.println("=== Cofre de Medicamentos ===");
    Serial.println("Para definir o horario, digite o tempo e sua respectiva unidade.");
}

int mediaMovelLDR() {
    int leituraNova = analogRead(ldr);

    leituraLDR[posLeitura] = leituraNova;
    posLeitura = posLeitura + 1;

    if (posLeitura >= 5) {
        posLeitura = 0;
    }

    int soma = 0;
    int i;
    for (i = 0; i < 5; i++) {
        soma = soma + leituraLDR[i];
    }

    return soma / 5;
}

void tocarMelodiaParabens() {
    tone(buzzer, 1500, 200);
    delay(250);
    tone(buzzer, 1800, 200);
    delay(250);
    tone(buzzer, 2200, 300);
    delay(350);
    noTone(buzzer);

    int i;
    for (i = 0; i < 3; i++) {
        digitalWrite(ledVerde, HIGH);
        delay(200);
        digitalWrite(ledVerde, LOW);
        delay(200);
    }
}

unsigned long converterParaSegundos(String texto) {
    unsigned long total = 0;

    int posHora = texto.indexOf("hora");
    if (posHora > 0) {
        String parteHora = texto.substring(0, posHora);
        parteHora.trim();
        int ultimoEspaco = parteHora.lastIndexOf(' ');
        if (ultimoEspaco >= 0) {
            parteHora = parteHora.substring(ultimoEspaco + 1);
        }
        total = total + (unsigned long)parteHora.toInt() * 3600;
    }

    int posMin = texto.indexOf("minuto");
    if (posMin > 0) {
        String parteMin = texto.substring(0, posMin);
        parteMin.trim();
        int ultimoEspaco = parteMin.lastIndexOf(' ');
        if (ultimoEspaco >= 0) {
            parteMin = parteMin.substring(ultimoEspaco + 1);
        }
        total = total + (unsigned long)parteMin.toInt() * 60;
    }

    int posSeg = texto.indexOf("segundo");
    if (posSeg > 0) {
        String parteSeg = texto.substring(0, posSeg);
        parteSeg.trim();
        int ultimoEspaco = parteSeg.lastIndexOf(' ');
        if (ultimoEspaco >= 0) {
            parteSeg = parteSeg.substring(ultimoEspaco + 1);
        }
        total = total + (unsigned long)parteSeg.toInt();
    }

    return total;
}

void verificarSerial() {
    if (Serial.available() > 0) {
        String comando = Serial.readStringUntil('\n');
        comando.trim();

        if (etapaConfiguracao == 0) {
            unsigned long segundos = converterParaSegundos(comando);

            if (segundos > 0) {
                tempoPendente = segundos;
                etapaConfiguracao = 1;
                Serial.println("Gostaria de colocar para toca-lo quantas vezes? Em caso de apenas uma vez, digite 1.");
            } else {
                Serial.println("Nao entendi. Tente assim:");
                Serial.println("Ex: 6");
            }

        } else if (etapaConfiguracao == 1) {
            int quantidade = comando.toInt();

            if (quantidade > 0) {
                intervaloDoses = tempoPendente;
                totalDoses = quantidade;
                dosesRestantes = quantidade;

                if (quantidade == 1) {
                    loopAtivo = 0;
                } else {
                    loopAtivo = 1;
                }

                horarioRemedio = (millis() / 1000) + intervaloDoses;
                horarioConfigurado = 1;
                alarmeDisparado = 0;
                mensagemAlarme = 0;
                statusTomou = 0;
                etapaConfiguracao = 0;

                if (quantidade == 1) {
                    Serial.print("Ok! O alarme vai tocar em ");
                    Serial.print(intervaloDoses);
                    Serial.println(" segundo(s).");
                } else {
                    Serial.print("Ok! ");
                    Serial.print(totalDoses);
                    Serial.print(" doses programadas, de ");
                    Serial.print(intervaloDoses);
                    Serial.println(" segundo(s) cada.");
                    Serial.print("Doses restantes: ");
                    Serial.println(dosesRestantes);
                }
            } else {
                Serial.println("Numero invalido. Digite quantas vezes quer que o alarme toque.");
            }
        }
    }
}

void loop() {
    horarioAtual = millis() / 1000;

    verificarSerial();

    if (horarioConfigurado == 1 && horarioAtual >= horarioRemedio && statusTomou == 0) {
        digitalWrite(ledVermelho, HIGH);
        tone(buzzer, 1000);

        if (alarmeDisparado == 0) {
            alarmeDisparado = 1;
            tempoInicioAlarme = millis() / 1000;
            if (loopAtivo == 1) {
                Serial.print("Hora da dose! Doses restantes: ");
                Serial.println(dosesRestantes);
            } else {
                Serial.println("Hora do remedio! Abra o cofre e pressione o botao.");
            }
        }
    }

    if (alarmeDisparado == 1 && statusTomou == 0) {
        int luzMedia = mediaMovelLDR();
        int sensibilidade = analogRead(potenciometro);

        int cofreAberto = 0;
        if (luzMedia > sensibilidade) {
            cofreAberto = 1;

            if (mensagemAlarme == 0) {
                mensagemAlarme = 1;
                Serial.println("Cofre aberto! Pressione o botao para confirmar.");
            }
        }

        int botaoAtual = digitalRead(botaoConfirma);

        if (cofreAberto == 0 && botaoAtual == LOW) {
            while (digitalRead(botaoConfirma) == LOW) {
                delay(10);
            }
            delay(50);
            Serial.println("O cofre ainda nao foi aberto. Para poder contabilizar, e necessário que, primeiro, voce abra a caixa.");
        }

        if (cofreAberto == 1 && botaoAtual == LOW) {
            while (digitalRead(botaoConfirma) == LOW) {
                delay(10);
            }
            delay(50);

            noTone(buzzer);
            digitalWrite(ledVermelho, LOW);
            digitalWrite(ledVerde, HIGH);

            pontos = pontos + 10;
            statusTomou = 1;
            alarmeDisparado = 0;
            mensagemAlarme = 0;

            Serial.print("Dose tomada! Pontos: ");
            Serial.print(pontos);

            if (loopAtivo == 1) {
                dosesRestantes = dosesRestantes - 1;

                if (dosesRestantes == 0) {
                    diasConsecutivos = diasConsecutivos + 1;
                    Serial.print(" | Dias seguidos: ");
                    Serial.println(diasConsecutivos);

                    if (diasConsecutivos % 3 == 0) {
                        pontos = pontos + (pontos / 2);
                        tocarMelodiaParabens();
                        Serial.print("Bonus! Pontos agora: ");
                        Serial.println(pontos);
                    }

                    delay(3000);
                    digitalWrite(ledVerde, LOW);

                    loopAtivo = 0;
                    horarioConfigurado = 0;
                    statusTomou = 0;

                    Serial.println("Todas as doses do dia foram tomadas!");
                    Serial.println("=== Cofre de Medicamentos ===");
                    Serial.println("Para definir o horario, digite o tempo e sua respectiva unidade.");
                } else {
                    Serial.print(" | Doses restantes: ");
                    Serial.println(dosesRestantes);

                    delay(3000);
                    digitalWrite(ledVerde, LOW);

                    horarioRemedio = (millis() / 1000) + intervaloDoses;
                    horarioConfigurado = 1;
                    statusTomou = 0;

                    Serial.print("Proxima dose em ");
                    Serial.print(intervaloDoses);
                    Serial.println(" segundo(s).");
                }
            } else {
                diasConsecutivos = diasConsecutivos + 1;
                Serial.print(" | Dias seguidos: ");
                Serial.println(diasConsecutivos);

                if (diasConsecutivos % 3 == 0) {
                    pontos = pontos + (pontos / 2);
                    tocarMelodiaParabens();
                    Serial.print("Bonus! Pontos agora: ");
                    Serial.println(pontos);
                }

                delay(3000);
                digitalWrite(ledVerde, LOW);

                horarioConfigurado = 0;
                statusTomou = 0;

                Serial.println("=== Cofre de Medicamentos ===");
                Serial.println("Para definir o horario, digite o tempo e sua respectiva unidade.");
            }
        }

        if ((millis() / 1000) - tempoInicioAlarme >= 30) {
            noTone(buzzer);
            digitalWrite(ledVermelho, LOW);

            Serial.println("Sequencia perdida! Os seus pontos foram mantidos.");
            diasConsecutivos = 0;

            alarmeDisparado = 0;
            mensagemAlarme = 0;
            horarioConfigurado = 0;
            statusTomou = 0;
            loopAtivo = 0;

            Serial.println("=== Cofre de Medicamentos ===");
            Serial.println("Para definir o horario, digite o tempo e sua respectiva unidade.");
        }
    }

    delay(100);
}