# Clean
rm -rf rethinkdb.js
rm -rf js_server.js

# Copy the JS driver
cp ../build/drivers/javascript/rethinkdb.js ./


# Compile the coffeescript server
coffee --compile js_server.coffee


# All the stuff for the dataexplorer
rm temp/main.coffee
cat dataexplorer/assets/coffee/dataexplorer_app.coffee >> temp/main.coffee
cat js_server.coffee >> temp/main.coffee
cat dataexplorer/assets/coffee/dataexplorer.coffee >> temp/main.coffee

coffee --compile --output dataexplorer/assets/js/  temp/main.coffee

lessc dataexplorer/assets/css/styles.less > dataexplorer/assets/css/styles.css
