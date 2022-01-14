modprobe msr

export BIN_DIR="build/bin"

pushd $BIN_DIR

PCM_NO_PERF=1 ./pcm -r -- sleep 1
if [ "$?" -ne "0" ]; then
   echo "Error in pcm"
   exit 1
fi

PCM_USE_UNCORE_PERF=1 ./pcm -r -- sleep 1
if [ "$?" -ne "0" ]; then
   echo "Error in pcm"
   exit 1
fi

./pcm -r -- sleep 1
if [ "$?" -ne "0" ]; then
   echo "Error in pcm"
   exit 1
fi

./pcm-memory -- sleep 1
if [ "$?" -ne "0" ]; then
    echo "Error in pcm-memory"
    exit 1
fi

./pcm-memory -rank=1 -- sleep 1
if [ "$?" -ne "0" ]; then
    echo "Error in pcm-memory"
    exit 1
fi

./pcm-memory -rank=1 -csv -- sleep 1
if [ "$?" -ne "0" ]; then
    echo "Error in pcm-memory"
    exit 1
fi

./pcm-iio -i=1
if [ "$?" -ne "0" ]; then
    echo "Error in pcm-iio"
    exit 1
fi

./pcm-raw -e core/config=0x30203,name=LD_BLOCKS.STORE_FORWARD/ -e cha/config=0,name=UNC_CHA_CLOCKTICKS/ -e imc/fixed,name=DRAM_CLOCKS  -- sleep 1
if [ "$?" -ne "0" ]; then
    echo "Error in pcm-raw"
    exit 1
fi

./pcm-mmio 0x0
if [ "$?" -ne "0" ]; then
    echo "Error in pcm-mmio"
    exit 1
fi

./pcm-pcicfg 0 0 0 0 0
if [ "$?" -ne "0" ]; then
    echo "Error in pcm-pcicfg"
    exit 1
fi

./pcm-numa -- sleep 1
if [ "$?" -ne "0" ]; then
    echo "Error in pcm-numa"
    exit 1
fi

./pcm-core -e cpu/umask=0x01,event=0x0e,name=UOPS_ISSUED.STALL_CYCLES/ -- sleep 1
if [ "$?" -ne "0" ]; then
    echo "Error in pcm-core"
    exit 1
fi

./examples/c_example
if [ "$?" -ne "0" ]; then
    echo "Error in c_example"
    exit 1
fi

./examples/c_example_shlib
if [ "$?" -ne "0" ]; then
    echo "Error in c_example_shlib"
    exit 1
fi

./pcm-msr -a 0x30A
if [ "$?" -ne "0" ]; then
    echo "Error in pcm-msr"
    exit 1
fi

./pcm-power -- sleep 1
if [ "$?" -ne "0" ]; then
    echo "Error in pcm-power"
    exit 1
fi

./pcm-pcie -- sleep 1
if [ "$?" -ne "0" ]; then
    echo "Error in pcm-pcie"
    exit 1
fi

./pcm-latency -i=1
if [ "$?" -ne "0" ]; then
    echo "Error in pcm-latency"
    exit 1
fi

./pcm-tsx -- sleep 1
if [ "$?" -ne "0" ]; then
    echo "Error in pcm-tsx"
    exit 1
fi

# TODO add more tests
# e.g for ./pcm-sensor-server, ./pcm-sensor, ...

./tests/urltest
# We have 2 expected errors, anything else is a bug
if [ "$?" != 2 ]; then
    echo "Error in urltest, 2 expected errors but found $?!"
    exit 1
fi

### Check pcm-raw with event files
# Download nescessary files
if [ ! -f "mapfile.csv" ]; then
    echo "Downloading https://download.01.org/perfmon/mapfile.csv"
    wget -q --timeout=10 https://download.01.org/perfmon/mapfile.csv
    if [ "$?" -ne "0" ]; then
        echo "Could not download mapfile.csv"
        exit 1
    fi
fi

VENDOR=$(lscpu | grep "Vendor ID:" | awk '{print $3}')
FAMILY=$(lscpu | grep "CPU family:" | awk '{print $3}')
MODEL=$(lscpu | grep "Model:" | awk '{printf("%x", $2)}')
STRING="${VENDOR}-${FAMILY}-${MODEL}-"
FILES=$(grep $STRING "mapfile.csv" | awk -F "\"*,\"*" '{print $3}')
DIRS=

for FILE in $FILES
do
    DIR="$(dirname $FILE)"
    DIR="${DIR#?}"
    if [[ ! " ${DIRS[*]} " =~ " ${DIR} " ]]; then
        DIRS+="${DIR} "
    fi
done

for DIR in $DIRS
do
    if [ ! -d $DIR ]; then
        mkdir $DIR
        cd $DIR

        DIRPATH="https://download.01.org/perfmon/${DIR}/"
        echo "Downloading all files from ${DIRPATH}"

        wget -q --timeout=10 -r -l1 --no-parent -A "*.json" $DIRPATH
        if [ "$?" -ne "0" ]; then
            cd ..
            echo "Could not download $DIR"
            exit 1
        fi
        wget -q --timeout=10 -r -l1 --no-parent -A "*.tsv" $DIRPATH
        mv download.01.org/perfmon/${DIR}/* .
        rm -rf download.01.org
        cd ..
    fi
done

# Now check pcm-raw with JSON files from mapFile.csv
./pcm-raw -r -e LD_BLOCKS.STORE_FORWARD -e CPU_CLK_UNHALTED.THREAD_ANY -e INST_RETIRED.ANY  -- sleep 1

if [ "$?" -ne "0" ]; then
    echo "Error in pcm-raw"
    exit 1
fi

# Now get corresponding TSV files and replace JSON files in mapFile.csv with them
cp "mapfile.csv" "mapfile.csv_orig"
for FILE in $FILES
do
    DIR="$(dirname $FILE)"
    DIR="${DIR#?}"
    cd $DIR
    BASE="$(basename $FILE)"
    TYPE="$(echo $BASE | sed 's/_v[0-9].*json//g')"
    # TYPE can be for example: skylakex_core or skylakex_uncore.
    CMD="find . -type f -regex '\.\/${TYPE}_v[0-9]*\.[0-9]*.tsv'"
    TSVFILE=$(eval $CMD)
    TSVFILE="${TSVFILE:2}"
    cd ..
    CMD="sed -i 's/${BASE}/${TSVFILE}/g' mapfile.csv"
    eval $CMD
done


# Check pcm-raw with TSV files
./pcm-raw -r -e LD_BLOCKS.STORE_FORWARD -e CPU_CLK_UNHALTED.THREAD_ANY -e INST_RETIRED.ANY  -- sleep 1

if [ "$?" -ne "0" ]; then
    echo "Error in pcm-raw"
    rm -rf mapfile.csv
    cp "mapfile.csv_orig" "mapfile.csv"
    exit 1
fi
rm -rf mapfile.csv
cp "mapfile.csv_orig" "mapfile.csv"

popd