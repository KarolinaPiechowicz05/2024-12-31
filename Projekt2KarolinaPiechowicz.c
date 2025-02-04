#include<sys/types.h>
#include<sys/socket.h>
#include<stdio.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>
#include<netdb.h>
#include<time.h>
#include<errno.h>
#include <signal.h>
#include <sys/time.h>

struct wiadomosc{
	unsigned char liczba_kontrolna;
	unsigned char nr_tury;
	unsigned char wartosc_dodana_przez_siebie;
	unsigned char wynik;
	char imie[16];
	char tekst[128];
};

struct addrinfo addrinfo_do_wysylania,addrinfo_do_odbierania, *wynik_wys, *wynik_odb;
char string_ip[INET_ADDRSTRLEN];
int status;
struct wiadomosc wiadomosc_wychodzaca;
struct wiadomosc wiadomosc_przychodzaca;
struct sockaddr_in *addr_drugiego_gracza;
int gniazdo_wys;
int gniazdo_odb;
int liczba_bajtow_przeslanych;
int liczba_bajtow_odebranych;
int nr_portu_do_wysylki;
int nr_portu_do_odbioru;
struct sockaddr_storage niewiemjaknazwactezmienna;
socklen_t dlugosc_adresu = sizeof niewiemjaknazwactezmienna;
struct timeval czas;
unsigned char nr_gracza = 0;
unsigned char nr_ostatniej_tury = 0;
unsigned char wynik_gry = 0;
unsigned char kto_zwyciezca;

//nie chce mi sie myslec wiec po prostu podkradne funkcje get_in_addr z listener'a beej'a. wlasciwie to i tak to tu jest zbedne bo nie wspiera ipv6 ale no
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void wypisz_wiadomosc(struct wiadomosc wiad){
	printf("[%s]: %s",wiad.imie,wiad.tekst);
}

//do chatu. liczba kontrolna = 5
void wyslij_wiadomosc(){
	printf("[GRA] Podaj wiadomosc do wyslania (jesli wiadomosc jest pusta, nie bedzie wyslana):\n> ");
	//kochac getline'a kochac (tekst ocenzurowany)
	//char *temp_tekst;
	//size_t rozmiar = (sizeof wiadomosc_wychodzaca.tekst)-1;
	//getline(&temp_tekst,&rozmiar,stdin);
	if (fgets(wiadomosc_wychodzaca.tekst, sizeof(wiadomosc_wychodzaca.tekst), stdin) == NULL){printf("[GRA] Blad fgets\n");exit(1);}
	wiadomosc_wychodzaca.liczba_kontrolna = 5;
	//printf("[GRA] Wiadomosc pobrana.\n");
	//snprintf(wiadomosc_wychodzaca.tekst, 254, "%s", temp_tekst);
	//free(temp_tekst);
	//printf("[GRA] Wiadomosc skopiowana do struktury: %s",wiadomosc_wychodzaca.tekst);
	if (strcmp(wiadomosc_wychodzaca.tekst,"") == 0 || strcmp(wiadomosc_wychodzaca.tekst,"\n") == 0){}
	else{
		liczba_bajtow_przeslanych = sendto(gniazdo_wys,&wiadomosc_wychodzaca,sizeof wiadomosc_wychodzaca,0,wynik_wys->ai_addr,wynik_wys->ai_addrlen);
		if (liczba_bajtow_przeslanych == -1){perror("[GRA] Blad sendto (chat)");exit(1);}
		//printf("[GRA] Wiadomosc wyslana.\n");
		}
}

void odbierz_wiadomosc(){
	liczba_bajtow_odebranych = recvfrom(gniazdo_odb,&wiadomosc_przychodzaca,sizeof wiadomosc_przychodzaca,0,(struct sockaddr *)&niewiemjaknazwactezmienna,&dlugosc_adresu);
	if (liczba_bajtow_odebranych == -1){perror("[GRA] blad recvfrom"); exit(1);}
	char s[INET_ADDRSTRLEN];
	//chat
	if (strcmp(string_ip,inet_ntop(niewiemjaknazwactezmienna.ss_family,
			get_in_addr((struct sockaddr *)&niewiemjaknazwactezmienna),
			s, sizeof s)) != 0){}
	else{
		//printf("[GRA] Liczba kontrolna: %d",wiadomosc_przychodzaca.liczba_kontrolna == 3);
		if (wiadomosc_przychodzaca.liczba_kontrolna == 5){
			//printf("[GRA] Odebrano wiadomosc:\n");
			wypisz_wiadomosc(wiadomosc_przychodzaca);
		}
		//wartosc podana przez gracza w grze
		else if (wiadomosc_przychodzaca.liczba_kontrolna == 3){
			wynik_gry = wiadomosc_przychodzaca.wynik;
		}
	}
}

void odbierz_wiadomosc_z_timeoutem(){
	czas.tv_sec = 0; czas.tv_usec = 5000;
	if (setsockopt(gniazdo_odb, SOL_SOCKET, SO_RCVTIMEO, &czas, sizeof(czas)) < 0) {perror("[GRA] setsockopt");exit(1);}
	
	liczba_bajtow_odebranych = recvfrom(gniazdo_odb,&wiadomosc_przychodzaca,sizeof wiadomosc_przychodzaca,0,(struct sockaddr *)&niewiemjaknazwactezmienna,&dlugosc_adresu);
	if (liczba_bajtow_odebranych == -1 && (errno == EAGAIN)){/*printf("[GRA] Nie przyszla zadna wiadomosc.\n");*/}
	else if (liczba_bajtow_odebranych == -1){perror("[GRA] Blad recvfrom"); exit(1);}
	else {
		//ignoruje wiadomosci od nieprawidlowych uzytkownikow
		char s[INET_ADDRSTRLEN];
		if (strcmp(string_ip,inet_ntop(niewiemjaknazwactezmienna.ss_family,
			get_in_addr((struct sockaddr *)&niewiemjaknazwactezmienna),
			s, sizeof s)) != 0){}
		else{
			/*printf("[GRA] liczba kontrolna = %d\n",wiadomosc_przychodzaca.liczba_kontrolna);*/
			if (wiadomosc_przychodzaca.liczba_kontrolna == 5){
			//printf("[GRA] Odebrano wiadomosc:\n");
			wypisz_wiadomosc(wiadomosc_przychodzaca);
		}
	//wartosc podana przez gracza w grze
			else if (wiadomosc_przychodzaca.liczba_kontrolna == 3){
			wynik_gry = wiadomosc_przychodzaca.wynik;
			}
		}
	}
	
	czas.tv_sec = 0; czas.tv_usec = 0;
	if (setsockopt(gniazdo_odb, SOL_SOCKET, SO_RCVTIMEO, &czas, sizeof(czas)) < 0) {perror("[GRA] setsockopt");exit(1);}
}

//liczba kontrolna = 4
void powiedz_drugiemu_graczowi_ze_jest_drugim_graczem(){
	wiadomosc_wychodzaca.liczba_kontrolna = 4;
	wiadomosc_wychodzaca.nr_tury = 0;
	wiadomosc_wychodzaca.wartosc_dodana_przez_siebie = 0;
	wiadomosc_wychodzaca.wynik = 0;
	strcpy (wiadomosc_wychodzaca.tekst,"");
	liczba_bajtow_przeslanych = sendto(gniazdo_wys,&wiadomosc_wychodzaca,sizeof wiadomosc_wychodzaca,0,wynik_wys->ai_addr,wynik_wys->ai_addrlen);
	if (liczba_bajtow_przeslanych == -1){perror("[GRA] Blad sendto (inicjalizacja gracz 1)");exit(1);}
	printf("[GRA] Przeslano drugiemu graczowi znak\n");
}

//liczba kontrolna = 2
void powiedz_pierwszemu_graczowi_ze_jestem_drugim_graczem(){
	wiadomosc_wychodzaca.liczba_kontrolna = 2;
	wiadomosc_wychodzaca.nr_tury = 0;
	wiadomosc_wychodzaca.wartosc_dodana_przez_siebie = 0;
	wiadomosc_wychodzaca.wynik = 0;
	strcpy (wiadomosc_wychodzaca.tekst,"");
	liczba_bajtow_przeslanych = sendto(gniazdo_wys,&wiadomosc_wychodzaca,sizeof wiadomosc_wychodzaca,0,wynik_wys->ai_addr,wynik_wys->ai_addrlen);
	if (liczba_bajtow_przeslanych == -1){perror("[GRA] Blad sendto (inicjalizacja gracz 2)");exit(1);}
	printf("[GRA] Przeslano pierwszemu graczowi znak spowrotem\n");
}

void wyznacz_pierwszego_gracza(){
	//ustala timeout recv na 3 sekund
	czas.tv_sec = 3; czas.tv_usec = 0;
	if (setsockopt(gniazdo_odb, SOL_SOCKET, SO_RCVTIMEO, &czas, sizeof(czas)) < 0) {perror("[GRA] setsockopt");exit(1);}
	
	liczba_bajtow_odebranych = recvfrom(gniazdo_odb,&wiadomosc_przychodzaca,sizeof wiadomosc_przychodzaca,0,(struct sockaddr *)&niewiemjaknazwactezmienna,&dlugosc_adresu);
	
	if (liczba_bajtow_odebranych == -1 && (errno == EAGAIN)){
		printf("[GRA] Nie znaleziono drugiego gracza. Jestes graczem nr 1. Jesli tak nie powinno byc, sprobuj ponownie.\n");
		powiedz_drugiemu_graczowi_ze_jest_drugim_graczem();
		nr_gracza = 0;
		printf("[GRA] Czekam na odpowiedz...\n");
		
		liczba_bajtow_odebranych = recvfrom(gniazdo_odb,&wiadomosc_przychodzaca,sizeof wiadomosc_przychodzaca,0,(struct sockaddr *)&niewiemjaknazwactezmienna,&dlugosc_adresu);
		if (liczba_bajtow_odebranych == -1 && (errno == EAGAIN)){printf("[GRA] Nie znaleziono drugiego gracza. Sprobuj ponownie.\n");exit(1);}
		else if (liczba_bajtow_odebranych == -1){perror("[GRA] Blad recvfrom"); exit(1);}
		printf("[GRA] Odpowiedz odebrano! Mozna zaczynac rozgrywke.\n");
		
	}
	else if (liczba_bajtow_odebranych == -1){perror("[GRA] Blad recvfrom"); exit(1);}
	else{
		printf("[GRA] Odebrano znak... ");
		char s[INET_ADDRSTRLEN];
		printf("Oczekiwane IP: %s, otrzymane IP: %s",string_ip, inet_ntop(niewiemjaknazwactezmienna.ss_family, get_in_addr((struct sockaddr *)&niewiemjaknazwactezmienna), s, sizeof s));
		if (strcmp(string_ip,inet_ntop(niewiemjaknazwactezmienna.ss_family,
			get_in_addr((struct sockaddr *)&niewiemjaknazwactezmienna),
			s, sizeof s)) != 0){printf(" ...Niezgodne! Zamykam.\n");exit(1);}
		printf(" ...Zgodne!\n");
		printf("[GRA] Sprawdzam zgodnosc znaku...");
		if (wiadomosc_przychodzaca.liczba_kontrolna != 4 || wiadomosc_przychodzaca.nr_tury != 0 || wiadomosc_przychodzaca.wartosc_dodana_przez_siebie != 0 || wiadomosc_przychodzaca.wynik != 0){printf(" ...Niezgodny! Zamykam.\n");exit(1);}
		printf(" ...Zgodny!\n");
		printf("[GRA] Odnaleziono pierwszego gracza. Jestes graczem nr 2.\n");
		nr_gracza = 1;
		powiedz_pierwszemu_graczowi_ze_jestem_drugim_graczem();
	}
	czas.tv_sec = 0; czas.tv_usec = 0;
	if (setsockopt(gniazdo_odb, SOL_SOCKET, SO_RCVTIMEO, &czas, sizeof(czas)) < 0) {perror("[GRA] setsockopt");exit(1);}
}

void wylosuj_liczbe_poczatkowa(){
	printf("[GRA] Losuje poczatkowa liczbe...");
	wiadomosc_wychodzaca.wynik = (rand() % 10) + 1;
	wynik_gry = wiadomosc_wychodzaca.wynik;
	printf(" ...Wylosowano! To %d ;)\n",wiadomosc_wychodzaca.wynik);
}

void faza_odbierania(){
	wiadomosc_przychodzaca.liczba_kontrolna = 0;
	printf("[GRA] Oczekuje na odpowiedz od wspolgracza. W miedzyczasie mozesz do niego wysylac wiadomosci.\n");
	while (wiadomosc_przychodzaca.liczba_kontrolna != 3){
		odbierz_wiadomosc_z_timeoutem();
		wyslij_wiadomosc();
	}
}

//liczba kontrolna = 3 (dla wysylania liczby)
void faza_nadawania(){
	//printf("[GRA] Obecny numer tury: %d\n",nr_ostatniej_tury);
	int liczba = 0; 
	while (liczba == 0){
		//printf("[GRA] Weszla w while, liczba=%d\n",liczba);
		printf("[GRA] Podaj liczbe od 1-10 by przeslac wspolgraczowi, albo 11 by wyslac wiadomosc przez chat.\n> ");
		scanf("%d",&liczba);
		getchar();
		if (liczba == 11)   {wyslij_wiadomosc();   liczba = 0;   }
		else if(liczba >= 1 && liczba <= 10){
			wiadomosc_wychodzaca.liczba_kontrolna = 3;
			wynik_gry += liczba;
			wiadomosc_wychodzaca.wynik = wynik_gry;
			wiadomosc_wychodzaca.wartosc_dodana_przez_siebie = liczba;
			nr_ostatniej_tury += 1;
			wiadomosc_wychodzaca.nr_tury = nr_ostatniej_tury;
			printf("[GRA] Wynik gry: %d\n",wynik_gry);
			liczba_bajtow_przeslanych = sendto(gniazdo_wys,&wiadomosc_wychodzaca,sizeof wiadomosc_wychodzaca,0,wynik_wys->ai_addr,wynik_wys->ai_addrlen);
			if (liczba_bajtow_przeslanych == -1){perror("[GRA] Blad sendto (nadawanie)");exit(1);}
		}
		else {printf("[GRA] Nieprawidlowa liczba: %d!\n",liczba); liczba = 0;}
		while (liczba_bajtow_odebranych != -1){odbierz_wiadomosc_z_timeoutem();}
		liczba_bajtow_odebranych = 0;
	}
	
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------------*/


// https://beej.us/guide/bgnet/html/split/
//UDP = User Datagram Protocol


//argc - ilosc argumentow, nazwa programu liczona jest jako 1 
//argv to to, jakie sa argumenty
//argv[1] - adres domenowy lub IP hosta
//argv[2] - numer portu
//argv[3] - opcjonalny nick, jesli nie byl podany to nickiem ma byc IP hosta

int main(int argc, char **argv){
	srand(time(NULL));
	if (argc != 3 && argc != 4){printf("[GRA] Nieprawidlowa ilosc argumentow: %d. Oczekiwano 3 lub 4.\n",argc);exit(1);}
	if (argc == 4 && strlen(argv[3]) > 15){printf("[GRA] Nieprawidlowa dlugosc nicku: %zu. Oczekiwano nie wieksza niz 15.\n",strlen(argv[3]));exit(1);}
	nr_portu_do_wysylki = atoi(argv[2]);
	if (nr_portu_do_wysylki <= 1024){printf("[GRA] Podany nr portu %d jest za niski, oczekiwano wiekszych od 1024.\n",nr_portu_do_wysylki);exit(1);}
	
	memset(&addrinfo_do_wysylania, 0, sizeof addrinfo_do_wysylania);
	memset(&addrinfo_do_odbierania, 0, sizeof addrinfo_do_odbierania);
	
	addrinfo_do_wysylania.ai_family = AF_INET;
	addrinfo_do_wysylania.ai_socktype = SOCK_DGRAM;
	
	addrinfo_do_odbierania.ai_family = AF_INET;
	addrinfo_do_odbierania.ai_socktype = SOCK_DGRAM;
	addrinfo_do_odbierania.ai_flags = AI_PASSIVE;
		
	//wyznacza IP jesli argv[1] to adres domenowy. znaczy, funkcja dziala i gdy [1] to ip, ale nie ma rozroznienia, i tak wyznacza ip
	
	if ((status = getaddrinfo(argv[1], argv[2], &addrinfo_do_wysylania, &wynik_wys)) != 0) {fprintf(stderr, "[GRA] Blad getaddrinfo: %s\n", gai_strerror(status));exit(1);}
	if ((status = getaddrinfo(NULL, argv[2], &addrinfo_do_odbierania, &wynik_odb)) != 0) {fprintf(stderr, "[GRA] Blad getaddrinfo: %s\n", gai_strerror(status));exit(1);}
	
	//wydziela ip z wyznaczonego 
	addr_drugiego_gracza = (struct sockaddr_in *)wynik_wys->ai_addr;
	if (inet_ntop(AF_INET,&(addr_drugiego_gracza->sin_addr),string_ip,INET_ADDRSTRLEN) == NULL){perror("[GRA] Blad inet_ntop");exit(1);}
	//ustala nick uzytkownika uzywany w czacie
	if (argc == 3){strcpy(wiadomosc_wychodzaca.imie,string_ip);}
	else{strcpy(wiadomosc_wychodzaca.imie,argv[3]);}
	//printf("[GRA] IP: %s\n", string_ip);
	gniazdo_odb = socket(wynik_odb->ai_family, wynik_odb->ai_socktype,wynik_odb->ai_protocol);
	gniazdo_wys = socket(wynik_wys->ai_family, wynik_wys->ai_socktype,wynik_wys->ai_protocol);
	if (gniazdo_odb == -1 || gniazdo_wys == -1){perror("[GRA] Blad socket");exit(1);}
	//patrzac na beej'a, bind() jest tylko w listenerze, nie w talkerze? huh
	if (bind(gniazdo_odb,wynik_odb->ai_addr,wynik_odb->ai_addrlen) == -1){perror("[GRA] Blad bind");exit(1);}
		
	/*
	https://man7.org/linux/man-pages/man2/connect.2.html
	If the socket sockfd is of type SOCK_DGRAM, then addr is the address to which datagrams are sent by default, and the only address from which datagrams are received.  If the socket is of type SOCK_STREAM or SOCK_SEQPACKET, this call attempts to make a connection to the socket that is bound to the address specified by addr.
	
	wiec chyba mozna uzywac connect do UDP? chociaz wsm
	https://stackoverflow.com/questions/9741392/can-you-bind-and-connect-both-ends-of-a-udp-connection
	
	Seems a bit strange, but I don't see why not. connect() just sets the default destination address/port for the socket. (Have you tried it? If it doesn't work for some reason, just use sendto().) Personally I'd just use sendto() because otherwise you'll get confused if multiple clients connect to your server. – mpontillo
	*/
	printf("[GRA] Witamy w grze! Oczekujemy na drugiego gracza przez 3 sekundy...\n");
	wyznacz_pierwszego_gracza();
	
	if (nr_gracza == 0){
		wylosuj_liczbe_poczatkowa();
		while (wynik_gry < 50){
			faza_nadawania();
			if (wynik_gry >= 50){kto_zwyciezca = 1; break;}
			faza_odbierania();
			if (wynik_gry >= 50){kto_zwyciezca = 0; break;}
		}
	}
	else{
		nr_ostatniej_tury = 1;
		while (wynik_gry < 50){
			faza_odbierania();
			if (wynik_gry >= 50){kto_zwyciezca = 2; break;}
			faza_nadawania();
			if (wynik_gry >= 50){kto_zwyciezca = 3; break;}
		}
	}
	printf("[GRA] Liczba wyniosla 50. Wygral");
	if (kto_zwyciezca % 2 == 1){printf("es Ty!\n");}
	else{printf(" Twoj wspolgracz!\n");}
	printf("[GRA] Zakonczono gre.\n");
	
	
	
	
	close(gniazdo_odb);
	

return 0;}
