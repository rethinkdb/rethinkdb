cd "$(dirname "$0")"
echo "Testing in Rhino..."
rhino -opt -1 tests.js
echo "Testing in Ringo..."
ringo -o -1 tests.js
export NARWHAL_OPTIMIZATION=-1
for cmd in narwhal node; do
	echo "Testing in $cmd..."
	$cmd tests.js
done
echo "Testing in a browser..."
open index.html