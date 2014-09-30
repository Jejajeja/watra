Projekt tłumaczenia protokołu portu szeregowego na protokół TCP/IP dla firmy Watra.
Twórca: Michał B.


Wstęp 

Kompilacja kodu źródłowego:
$ gcc main_fc.c create_frames.c cyclic_buffer.c main.c -lptherad -o tlumacz
Uruchamianie programu 

$ ./tlumacz [parametry]
Spis parametrów:


-h                 pomoc
-p                 włączenie kontroli                                         (domyślnie wyłączona)
-s                 włączenie bitu stopu                                       (domyślnie wyłączony)
-d [adres portu]   adres portu szeregowego na którym translator ma nasłuchiwać(domyślnie /dev/ttyAMA0)
-b [baudrate]      określenie szybkości transmisji przez port szeregowy       (domyślnie 9600)
-i [adres IP]      określenie adresu IP Integratora                           (domyślnie 192.168.1.2)
-q [port]          określenie numeru portu Inteagratora                       (domyślnie 26000)
-w [kod nadawcy]   kod urządzenia podany w systemie dziesiętnym               (domyślnie 1212)
-o [kod odbiorcy]  kod Integratora                                            (domyślnie 6969)

Sposób działania

Program działa na 2 wątkach wzajemnie asynchronicznych. Pierwszy (thThread_check_frames) na czas swojego działania blokuje drugi wątek sprawdza czy w buforze ramek oczeukjących na potwierdzenie został przekroczony maksymalny czas oczekiwania i jeśli tak to ponawia nadanie ramki aż do przekroczenia maksymalnej wartości powtórzeń.  Po zakończeniu działania odblokowuje drugi wątek i zatrzymuje się na czas 0.5s



Drugi (thReciveFrame) odpowiada za nasłuch na portach szeregowym i Ethernet i oczeuje na ramkę którą następnie przetwarza i przesyła odpowienio do Integratora lub urządzenia.


