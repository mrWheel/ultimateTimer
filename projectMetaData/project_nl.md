# Ultimate Timer

Ultimate Timer is een timercontroller op basis van een ESP32 voor projecten die een betrouwbare en herhaalbare besturing van een uitgang nodig hebben. De firmware is ontworpen voor een ESP32 Dev Module met een 2,4-inch SPI-TFT-scherm en een EC11 rotary encoder-module. De timer kan lokaal via het display worden bediend en via WiFi ook vanuit een browser met de Web UI worden ingesteld.

## Timerstanden

De firmware ondersteunt twee verschillende timermodellen. Een cyclische timer wisselt tussen instelbare AAN- en UIT-fasen. De timer kan een vast aantal cycli uitvoeren of onbeperkt blijven herhalen. De duur van beide fasen kan worden opgegeven in milliseconden, seconden of minuten. Ook de triggerstand, triggerflank, uitgangspolariteit en het blokkeren van ingangen tijdens het draaien zijn instelbaar.

De 24-uurtimer is bedoeld voor schema's die iedere dag opnieuw worden uitgevoerd. De dag is verdeeld in 96 blokken van een kwartier. Elk blok kan UIT, AAN of een willekeurige overgang naar AAN of UIT bevatten. De runtime bepaalt de actuele toestand aan de hand van de lokale klok en toont het mogelijke overgangsvenster op het statusscherm. Een 24-uursprofiel mag alleen draaien wanneer de systeemtijd een geldige datum en tijd bevat. Na synchronisatie van de tijd start het profiel automatisch op de juiste positie in het dagschema.

## Bedieningsmogelijkheden

De TFT-interface wordt bediend met de rotary encoder en de drukknop daarvan. Met een lange druk wordt het lokale menu geopend en met een korte druk wordt een menuoptie geselecteerd. Het menu bevat aparte editors voor cyclische timers en 24-uurtimers, profielbeheer, systeeminstellingen, WiFi-configuratie en displayinstellingen. Het statusscherm toont het actieve profiel, de timerstand, de uitgangstoestand, fase- of schema-informatie en waar mogelijk de resterende tijd.

Via de Web UI zijn dezelfde belangrijke instellingen vanuit een browser beschikbaar. Er zijn aparte editors voor cyclische timers en 24-uurtimers, inclusief een volledige editor met 24 uur en 4 kwartieren per uur. Daarnaast zijn er functies voor het laden, opslaan en verwijderen van profielen, systeeminstellingen en live statusinformatie. Instellingen kunnen direct worden toegepast, terwijl het opslaan in het profiel een bewuste aparte actie blijft. De interface laat zien wanneer de actieve instellingen afwijken van het opgeslagen profiel.

## Profielen en opslag

Timerprofielen worden als JSON-bestanden opgeslagen in LittleFS. Cyclische profielen gebruiken de profielnaam als bestandsnaam; 24-uursprofielen krijgen automatisch het achtervoegsel `-24h`. De naam van het actieve profiel wordt opgeslagen in ESP32 Preferences (NVS), zodat na een reset of herstart hetzelfde profiel opnieuw kan worden geladen. Bij het opstarten worden ook de systeeminstellingen hersteld. Een cyclische timer blijft daarna gestopt, terwijl een 24-uurtimer start zodra een geldige tijd beschikbaar is.

Een profiel bevat het timertype, de waarden en eenheden van de cyclische timer, het aantal herhalingen, triggerinstellingen en alle 96 kwartierwaarden van een 24-uurschema. Systeeminstellingen zoals uitgangspolariteit, encoder-richting, kleurenthema, displayrotatie, WiFi-status en automatisch opslaan worden afzonderlijk bewaard. De ingebouwde standaardprofielen kunnen niet worden verwijderd. Wanneer het actieve profiel wordt verwijderd, wordt automatisch het passende standaardprofiel geladen.

## Hardware en software

Het project wordt gebouwd met PlatformIO en het Arduino-framework voor ESP32. Het gebruikt onder andere LittleFS, Preferences, ArduinoJson, WiFiManager en de Adafruit graphics- en displaybibliotheken. De broncode is opgesplitst in modules voor de timerengine, profielbeheer, instellingenopslag, invoer, displaydriver, uitgangsbesturing, WiFi-beheer, lokaal menu en Web UI. De GPIO-aansluitingen, displayafmetingen, drukduur, standaardwaarden, logging en optionele kleurentest worden ingesteld via `platformio.ini`.

## Veiligheid

Deze firmware kan hardware besturen die verbonden is met gevaarlijke spanningen, hoge temperaturen, motoren, verwarmingen, lampen of andere risicovolle apparatuur. De repository geeft geen elektrische veiligheidscertificering en garandeert niet dat een specifieke schakeling veilig is. Gebruik geschikte galvanische scheiding, zekeringen, behuizingen, aarding, trekontlasting en onafhankelijke hardwarematige beveiligingen. Alleen personen met voldoende technische kennis mogen de hardware bouwen of aansluiten. Vertrouw nooit uitsluitend op deze firmware als veiligheidsvoorziening.
