# Important marocco options to control the map and route steps

This file introduces some of the most important options, which can be used to control an experiment via marocco.
Marocco is used to map and route the network described in PyNN to the BrainScaleS-1 hardware.
It is implemented in C++ but some functionalities are wrapped to python.
This allows us to customize the map and route steps from python.
Basic usage examples can be found [here](https://electronicvisions.github.io/hbp-sp9-guidebook/pm/using_pm_newflow.html).
Additional information and settings that can be set via marocco in python can be found in the Doxygen file.

## Include marocco
To use marocco in your python script you first have to include it:
```python
from pymarocco import PyMarocco
marocco = PyMarocco()
```

## Choose wafer system
The wafer system used for the experiments can be specified with (default: WAFER=33):
```python
from pyhalco_hicann_v2 import Wafer
marocco.default_wafer = Wafer($WAFER)
```
The state of the systems can be observed in [Grafana](https://brainscales-r.kip.uni-heidelberg.de:12443/grafana/).

## Specify defect database
For the mapping algorithm to be aware of unavailable components you have to load a defect database.
There are different stages of defect files, which build up on each other (cf. [Commissioning](https://brainscales-r.kip.uni-heidelberg.de/projects/logbook-nmpm/wiki/commissioning)).
For experiments, the highest level `derived_plus_calib_plus_layer1_blacklisting` should be used.
If not available, fallback to `derived_plus_calib_blacklisting` or `derived_blacklisting`.\
Currently, the files are still WIP and stored in `/wang/data/commissioning/BSS-1/rackplace/$WAFER/`, where `$WAFER` is the number of the used wafer.
`current` points to the latest version.
Consequently, the latest version can be included using
```python
from pymarocco import Defects
marocco.defects.backend = Defects.Backend.XML
marocco.defects.path = /wang/data/commissioning/BSS-1/rackplace/$WAFER/derived_plus_calib_plus_layer1_blacklisting/current
```
It is also possible to generate custom defect files.
For this it is recommended to generate a local copy of the latest defect folder of the used wafer and adapt it via `redman_cli.py`.
Then replace the path of `marocco.defects.path` with your local folder.

## Specify calibration database
The current calibration can be found in `/wang/data/commissioning/BSS-1/rackplace/$WAFER/calibration/current`, where `$WAFER` is the number of the used wafer.
It is specified using:
```python
marocco.calib_backend = PyMarocco.CalibBackend.XML
marocco.calib_path = /wang/data/commissioning/BSS-1/rackplace/$WAFER/calibration/current
```
**Caution, `CalibBackend.Default` means no calibration is used!**

## Skip checks
Per default, marocco executes checks before an experiment is run on the hardware, to verify the usability of the used components.
If many runs of the same experiment are executed in sequence, **without** changing the mapping in between, these checks only have to be executed during the first run on the hardware.
To speed up subsequent experiments, the checks can be skipped using:
```python
marocco.verification = PyMarocco.Skip
marocco.checkl1locking = PyMarocco.SkipCheck
```

## Ignore synapse loss
Marocco tries to route all specified synapses.
If the routing algorithm is not able to implement all synapses it aborts the experiment execution with an error.
If lost synapses are acceptable the experiment can still be executed using
```python
marocco.continue_despite_synapse_loss = True
```
