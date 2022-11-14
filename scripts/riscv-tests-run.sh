BIN=build/emu
RISCV_TESTS_DIR=riscv-tests

r=$'\r'
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

for FILE in $(ls ${RISCV_TESTS_DIR}/isa/rv64u*-*-* | grep -v uf- | grep -v ua-v |  grep -v ud- | grep -v .dump)
do
    echo -n "[TEST] ${FILE}..."
    $BIN --binary $FILE --riscv-test > /dev/null 2>&1
    if [ $? != 0 ] ; then
        echo -e "${r}Run test: ${FILE}... ${RED}FAIL${NC}"
    else
        echo -e "${r}Run test: ${FILE}... ${GREEN}PASS${NC}"
    fi
done
