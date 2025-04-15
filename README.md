# Raspberry Pi Pico W WiFi Setup (C++)

Detta projekt visar hur du ställer in Raspberry Pi Pico W för att ansluta till ett WiFi-nätverk och skicka mätvärden via mqtt till en broker (raspberry Zero) med C++.

## Beroenden

* Raspberry Pi Pico SDK
* CMake
* Make

## Installation

1.  **Klona repot:**

```bash
    git clone [repo-länk]
```

2.  **Skapa headerfil med WiFi-uppgifter:**
```bash
    cat > include/wifi_credentials.h << EOF
    #pragma once
    #include <string>

    const std::string WIFI_SSID = "change to your SSID";
    const std::string WIFI_PASSWORD = "change to your WiFi password";
    EOF
```

    **Viktigt:** Byt ut `"change to your SSID"` och `"change to your WiFi password"` med dina faktiska WiFi-uppgifter. 
    
    **Var försiktig med att inte lägga upp denna fil på ett publikt git repo!**.

3.  **Konfigurera CMake:**
```bash
    cmake -B build
```

4.  **Bygg projektet med Make:**

```bash
    make -C build
```
    Den kompilerade `.uf2`-filen skapas i `build/bin`.

5. Koppla enligt schema 
```bash 
    lägg in en bild här!!
```

6.  **Bootload Pico W:**

    Sätt din Pico W i bootload-läge genom att hålla ner BOOTSEL-knappen och ansluta den till din dator. Kopiera eller flytta `.uf2`-filen till Pico W.

om du inte har satt upp Rasp Zero så kan ni kolla på detta repo
```bash
    lägg in repo här 
```

7. **SSH:a in i din rasp zero och installera mosquitto och mosquitto client**
    
```bash 
    sudo apt install mosquitto mosquitto-clients -y
``` 
8. Lägg in information i mosquitto conf för att kunna lyssna och välj rätt port.
```bash
    sudo vim /etc/mosquitto/mosquitto.conf
```
    lägg in detta längst ner i filen:
```bash 
    allow_anonymous true
    listener 1883 0.0.0.0
```
9. kör en subscribe på ämnet, visar att det är larm
    bygg vidare på denna så även info om enhet skickas med.. 

```bash 
    mosquitto_sub -t /motion/alarm
```
10. kör en till sub via ett annat terminalfönster, visar distancen vid larm 
```bash
    mosquitto_sub -t /motion/distance
```
11. kör en tredje sub via nytt fönster, visar en indikation om picon förlorar ström / går offline..
```bash 
    mosquitto_sub -t /alarm/offline -v

```
## Felhantering

* Om anslutningen misslyckas, kontrollera att ditt SSID och lösenord är korrekta.
* Om Pico W inte hittas på nätverket, kontrollera att den är korrekt bootloadad och att ditt WiFi-nätverk fungerar.
