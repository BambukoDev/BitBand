# Co to jest?
**BitBand** to przenośny mini-komputer na rękę, inspirowany Pip-Boy'em z gry Fallout.

# Jak on działa?
Rdzeniem całego projektu jest mikrokontroler Raspberry Pi Pico W, który odpowiada za komunikację z resztą podzespołów poprzez kernel biblioteki FreeRTOS.
Urządzenie zostało wyposażone w 5 przycisków i enkoder obrotowy, które umożliwiają poruszanie się i kontrolowanie go.
Projekt jest napisany w językach C i C++.

## Podzespoły
- Raspberry Pi Pico W
- wyświetlacz 20x4 znaki LCD
- 5 przycisków
- enkoder obrotowy z 20 krokami
- czytnik kart MicroSD
- zewnętrzny zegar RTC
- akcelerometr

## Jakie ma funkcjonalności?
- Możliwość programowania z komputera (aktualizacje)
- W przyszłości większość wpisanych "na twardo" funkcjonalności zostanie zastąpiona łatwiej modyfikowalnymi plikami .lua
### Menu główne
- Wyświetlanie czasu, temperatury i napięcia baterii
### Menu wyboru
- Pozwala na wybranie jednej z wielu dostępnych opcji za pomocą enkodera i przycisków

### Aplikacja mobilna
- Aktualnie jeszcze jest niedokończona, jednak w przyszłości pojawi się taka funkcjonalność

## Do czego można to użyć?

# Historia projektu
