Instrukcja kompilacji:
Aby skopilować projekt, należy uruchomić instrukcję 'make' w głównym folderze projektu. Pliki wynikiowe pojawią wię w folderze Res.

Instukcja uruchomienia:
Aby uruchomić grę, należy w jednym oknie terminala uruchomić program 'server.out', a w dwóch kolejnych klientów 'client.out'.


Opis zawartości plików *.c

server.c:
	plik zawierający kod programu serwera, realizuje wszystkie funkcje serwera opisane w treści zadania,
klient.c:
	plik zawierający kod programu klienta, realizuje wszystkie funkcje klienta opisane w treści zadania,

kbhit.c:
	plik zawierający implementację funkcji kbhit(), służącej do wykrywania naduszenia klawisza na klawiaturze,
myipc.c:
	plik zawierający dodatkowe fukncje wykorzysytywane w projekcie, bazujące głównie na pakiecie IPC, takie jak np. otwarcie kolejki komunikatów z pewnością że kolejka zostanie wyczyszczona,
my_semafors.c
	plik zawierający funkcje P(), V() oraz inicjującą semafory, napisane dla wygody użytkowania operacji semaforowych w projekcie,
