# Foreign Data Wrapper for XDBC Client and Server

This fdw interfaces with XDBC Client and XDBC Server, which establishes data transfer with PostgreSQL via XDBC.



## Install

Hi Hari :)

Als allererstes nach dem pullen dieses repo musst du die submodule initialisieren mit: `git submodule update --init --recursive
`dann gehts los:

Als erstes schreibst du in experimente/setup_fdw.sql deine foreign tables rein. Dort steht als kommentar ganz oben worauf du achten musst. Dann packst du die passenden schema files für alle xdbc tables in ressources/schemas. Achte darauf, dass in den xdbc tables in der option die richtigen dateien als pfad angegeben sind. Steht auch in der anleitung in setup_fdw.sql.

Danach gehst du ins docker verzeichnis und führst da make aus. Das baut den pg_xdbc_fdw_client mit xdbc, jdbc, postgres jdbc treiber, postgres_fdw. 

Dann nehm ein eigenes docker compose file, oder meines. Du meintest, du hast schon ein postgresdb container mit allen datasets. kannst den Postgres container in meiner compose file durch deinen ersetzen oder du machst es selber.

Wenn der pg_xdbc_fdw_client container und der xdbc-server container und ein Postgres container da sind, dann kannst du in der experiments/run_experiments_for_datasets.sh deine variablen eintragen. Dort steht auch am anfang ein kommentar, worauf man achten muss. Vorallem die Containernamen und die dataset namen sind wichtig.

Und dann führst du in dem xdbc client container die setup_fdw.sql datei mit psql aus, damit die fdws und foreign tables erstellt werden.

Nun kannst du die experimente starten lassen von außerhalb mit der experiments/run_experiments_for_datasets.sh datei. Der einzige Parameter der Datei ist, wie oft das select * für jeden table ausgeführt werden soll. Ich mach zur Zeit immer 10. Er schreibt dir dann die zeiten in eine datei und den durchschnitt in eine andere.