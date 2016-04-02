#!/bin/sh


for i in 3.5 3.6 3.7 3.8 3.9 4.0 4.1 4.2 4.3 4.4 4.5 4.6 4.7 4.8 4.9 5.0; do
	sed -i "s/POWER_NORM=.*/POWER_NORM=$i/g" config.properties
	./Debug/place_cells_retin
done
