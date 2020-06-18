Talmaciu Vlad

				       README
			   Algoritmi Paraleli si Distribuiti
			Tema 3 - Procesare de Imagini Folosind MPI
					
					
		1. Introducere
	A fost implementat un program ce poate aplica 5 filtre unei imagini in 
format PGM sau PNM.
	Programul foloseste MPI pentru a putea imparti sarcina de lucru mai multor
procese, iar la iesirea standard afiseaza timpul scurs de dupa citirea imaginii
de intrare pana inainte de scrierea imaginii de iesire (adica necesar aplicarii
filtrelor).

		2. Implementare
	2.1. Citirea imaginii de intrare
	Procesul 0 deschide fisierul binar de intrare si citeste metadatele de la 
inceputul sau, stocandu-le in vectorul meta_data astfel:
		meta_data[0] - 5 sau 6, in functie de identificatorul formatului
		meta_data[1] - width
		meta_data[2] - height
		meta_data[3] - maxval
		meta_data[4] - numarul de culori (1 pentru alb-negru, 3 pentru color)
	Se aloca un vector de dimensiunea (width + 2) * (height + 2) * color_no si 
se initializeaza la 0, dupa care se introduc valori din fisier incat sa fie
echivalentul imaginii bordate cu 0.
	Metadatele sunt trimise la restul proceselor si fiecare proces calculeaza
numarul de randuri pe care va aplica filtrele, alocand un vector echivalent cu
o matrice [(width + 2) * color_no][slice_height + 2] (unde primeste partea de
imagine pe care o va procesa + randurile de sus/jos) si un vector echivalent cu
[(width + 2) * color_no][slice_height] (unde stocheaza valorile modificate si
pe care il va trimite inapoi la procesul 0). Procesul 0 va aplica filtrele pe
primul bloc, procesul 1, pe al doilea etc. Daca numarul de randuri nu este
divizibil cu numarul de procese, ultimele (height % nProcesses) randuri vor fi
prelucrate de procesul 0, care va retine valorile calculate in leftover_slice.
	
	2.2. Aplicarea filtrelor
	Pentru fiecare filtru:
		a) procesul 0 trimite catre restul proceselor blocurile de imagine pe
		care le vor prelucra (copiaza si pentru propriul received_slice valorile
		necesare)
		b) procesele aplica filtrul curent pentru fiecare culoare a fiecarui 
		pixel din blocul primit
		b.i) procesul 0 aplica filtrul si pe leftover_slice
		c) procesele trimit slice_to_send la procesul 0, care va suprascrie
		aceste valori peste cele din unaltered_image, pixelii de dupa aplicarea
		filtrului luand locul celor de dinainte
		c.i) procesul 0 face acelasi lucru si cu valorile din propriul
		slice_to_send si din leftover_slice
		
	2.3. Scrierea imaginii de iesire
	La final, vectorul unaltered_image va contine pixelii imaginii pe care au
fost aplicate succesiv filtrele date ca parametri (bordata cu 0). Aceste valori
vor fi scrise in fisierul de iesire (dupa scrierea metadatelor) in acelasi mod
in care au fost citite la inceput (cate width * color_no odata).

	3. Evaluarea scalarii
	Pentru a analiza comportamentul de scalare, au fost alese cele mai mari
imagini de fiecare format (landscape.pnm si rorschach.pgm) si pentru fiecare
a fost masurat timpul necesar aplicarii filtrelor bssembssem, variind numarul
de procese si calculand timpul mediu dupa cate 5 rulari pentru fiecare caz.
	Timpii necesari citirii fisierului de intrare/scrierii fisierului de
iesire nu au fost luati in considerare. Testele au fost efectuate pe un
procesor cu 6 nuclee.

		Rorschach.pgm
		
		1 proces : 1.84s
		2 procese : 0.95s
		3 procese : 0.65s
		4 procese : 0.50s
		5 procese : 0.42s
		6 procese : 0.36s
		
		Landscape.pnm
		
		1 proces : 6.30s
		2 procese : 3.23s
		3 procese : 2.25s
		4 procese : 1.73s
		5 procese : 1.47s
		6 procese : 1.26s
		
	Timpul necesar aplicarii filtrelor este foarte aproape invers proportional
cu numarul proceselor. Motivul pentru aceasta este faptul ca, desi sarcina de
lucru pentru fiecare proces este invers proportionala cu numarul lor, odata cu
marirea numarului de procese se mareste si numarul de mesaje pe care procesul 0
trebuie sa le trimita/primeasca. De asemenea, punctul in care imaginea este
recompusa cu valorile primite de 0 de la celelalte procese actioneaza ca o
bariera in program (datorita naturii blocante a MPI_Recv dar si pentru a
asigura corectitudinea algoritmului), executia adoptand viteza celui mai lent
proces.	