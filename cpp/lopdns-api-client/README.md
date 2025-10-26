# C++ lopdns-api-client

## Build

Get the dependencies, for instance by cloning the repos and build them.

### restclient-cpp

```bash
git clone git@github.com:mrtazz/restclient-cpp.git
cd restclient-cpp
./autogen.sh
./configure 
sudo make install
```

### json

```bash
git clone git@github.com:nlohmann/json.git 
cd json/include
sudo cp -r nlohmann /usr/local/include
```

### args

```bash
git clone git@github.com:Taywee/args.git
cd args
sudo make install
```

## Run

Get help:

```bash
./lopdns-api-client -h
```

Get all zones:

```bash
./lopdns-api-client -c "<client-id>" -a get-zones
```

Get a specific zone:

```bash
./lopdns-api-client -c "<client-id>" -a get-zones -z "<zone>"
```

Get all records for a specific zone:

```bash
./lopdns-api-client -c "<client-id>" -a get-records -z "<zone>"
```
