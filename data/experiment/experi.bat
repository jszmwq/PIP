set experimentPath=E:\raster_join\PIP-master\data\experiment\experiment.csv
if not exist "%experimentPath%" (
    type nul > "%experimentPath%"
    echo File created: %experimentPath%
) else (
    echo File already exists: %experimentPath%
)

for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value') do set current_time=%%I
set current_time=%current_time:~0,4%-%current_time:~4,2%-%current_time:~6,2% %current_time:~8,2%:%current_time:~10,2%:%current_time:~12,2%
echo Current Time : %current_time% >> %experimentPath%

set num_points=10 100 1000 10000 100000 1000000
set polygonPath=E:\raster_join\PIP-master\data\polygons\new_plan_polygon.txt

rem normal
cd /d E:\raster_join\PIP-master
echo GPUPIP_normal >> %experimentPath%
call make
for %%a in (%num_points%) do (
    set pointPath=E:\raster_join\PIP-master\data\points\selectivity_new_normal_distribution_%%a.loc
    set parameter=--polyPath "%polygonPath%" --pointPath "%pointPath%" --outputResult --experimentPath "%experimentPath%" --nIter 10
    PIP %parameter%
)

cd /d E:\raster_join\PIP-master\data\experiment
echo RayCasting_normal >> %experimentPath%
g++ -fopenmp RayCasting.cpp -o RayCasting
for %%a in (%num_points%) do (
    set pointPath=E:\raster_join\PIP-master\data\points\selectivity_new_normal_distribution_%%a.loc
    set parameter="%pointPath%" "%polygonPath%" "%experimentPath%"
    RayCasting %parameter%
)

cd /d E:\raster_join\PIP-master\data\experiment
echo PostGIS_normal >> %experimentPath%
set parameter=prepare "%polygonPath%"
python Postgis.py %parameter%

for %%a in (%num_points%) do (
    set pointPath=E:\raster_join\PIP-master\data\points\selectivity_new_normal_distribution_%%a.loc
    set parameter=query "%polygonPath%" "%pointPath%" "%experimentPath%"
    python Postgis.py %parameter%
)

rem uniform
cd /d E:\raster_join\PIP-master
echo GPUPIP_uniform >> %experimentPath%
call make
for %%a in (%num_points%) do (
    set pointPath=E:\raster_join\PIP-master\data\points\selectivity_new_uniform_distribution_%%a.loc
    set parameter=--polyPath "%polygonPath%" --pointPath "%pointPath%" --outputResult --experimentPath "%experimentPath%" --nIter 10
    PIP %parameter%
)

cd /d E:\raster_join\PIP-master\data\experiment
echo RayCasting_uniform >> %experimentPath%
g++ -fopenmp RayCasting.cpp -o RayCasting
for %%a in (%num_points%) do (
    set pointPath=E:\raster_join\PIP-master\data\points\selectivity_new_uniform_distribution_%%a.loc
    set parameter="%pointPath%" "%polygonPath%" "%experimentPath%"
    RayCasting %parameter%
)

cd /d E:\raster_join\PIP-master\data\experiment
echo PostGIS_uniform >> %experimentPath%
set parameter=prepare "%polygonPath%"
python Postgis.py %parameter%

for %%a in (%num_points%) do (
    set pointPath=E:\raster_join\PIP-master\data\points\selectivity_new_uniform_distribution_%%a.loc
    set parameter=query "%polygonPath%" "%pointPath%" "%experimentPath%"
    python Postgis.py %parameter%
)