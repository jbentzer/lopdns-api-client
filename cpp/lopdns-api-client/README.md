# C++ lopdns-api-client

## Build

### Dependencies

Restore all submodules with: 

```bash
git submodule update --init --recursive
```

#### restclient-cpp

Might require curl dev to be installed:

```bash
sudo apt install libcurl4-openssl-dev -y
```

```bash
cd ./cpp/3rd-party/restclient-cpp
./autogen.sh
./configure 
sudo make install
sudo cp /usr/local/lib/librestclient-cpp.so.1 /usr/lib
```

#### Not required

##### json

```bash
cd ./cpp/3rd-party/json/include
sudo cp -r nlohmann /usr/local/include
```

##### args

```bash
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

Create a record in a specific zone:

```bash
./lopdns-api-client -c "<client-id>" -a create-record -z "<zone>" -n "<record name>" -t "<type>" -T "<ttl>" -p "<priority>" -w "<content>
```

Update a record for a specific zone, type, name and content:

```bash
./lopdns-api-client -c "<client-id>" -a update-record -z "<zone>" -r "<record type>" -n "<record name>" -u "<current contents>" -w "<new contents>"
```

Update all records in a zone with the specific name and type:
```bash
./lopdns-api-client -c "<client-id>" -a update-record -z "<zone>" -r "<record type>" -n "<record name>" -w "<new contents>" --all-records
```

Update a record for a specific zone, type, name and regex content by exchanging "API" with "REGEX" in the contents:
```bash
./lopdns-api-client -c "<client-id>" -a update-record -z "zone" -r "<record type>" -n "<record name>" -u "<regex to find matching content>" -x "<regex to extract replacement from content>" -w "(sub-)string to write as replacement content"
```
