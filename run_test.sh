: ${BOOST_ROOT:?"Please define BOOST_ROOT as the path to your boost directory (1.54.0)"}
echo "Generating orders.  Please wait..."
./OrderGenerator.pl > Orders.txt
echo "Building Binary."
make profile
echo "Performing Test.  Please wait..."
./OrderBookProcessor Orders.txt > test.out 2>&1
less -n test.out
