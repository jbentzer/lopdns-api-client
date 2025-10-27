# C++ lopdns-api-client

## Build

### Dependencies

#### restclient-cpp

Might require curl dev to be installed:

```bash
sudo apt install libcurl4-openssl-dev -y
```

```bash
git clone git@github.com:mrtazz/restclient-cpp.git
sudo apt-get install libcurl4-openssl-dev
cd restclient-cpp
./autogen.sh
./configure 
sudo make install
sudo cp /usr/local/lib/librestclient-cpp.so.1 /usr/lib
```

#### json

```bash
git clone git@github.com:nlohmann/json.git 
cd json/include
sudo cp -r nlohmann /usr/local/include
```

#### args

```bash
git clone git@github.com:Taywee/args.git
cd args
sudo make install
```

### Build

In the source code directory, type:

```bash
make
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

Update a record for a specific zone, type, name and content:

```bash
./lopdns-api-client -c "<client-id>" -a update-record -z "<zone>" -r "<record type>" -n "<record name>" -u "<current contents>" -w "<new contents>"
```

Update all records in a zone with the specific name and type:
```bash
./lopdns-api-client -c "<client-id>" -a update-record -z "<zone>" -r "<record type>" -n "<record name>" -w "<new contents>" --all-records
```
