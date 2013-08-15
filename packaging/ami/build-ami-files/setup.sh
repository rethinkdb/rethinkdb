set -ex

# install applications and libraries
add-apt-repository -y ppa:rethinkdb/ppa
sudo add-apt-repository ppa:chris-lea/node.js
aptitude -y update
aptitude install -y rethinkdb nginx-light apache2-utils python-twisted rubygems python-pip npm nodejs
aptitude -y safe-upgrade
npm install -g rethinkdb
pip install rethinkdb
gem install rethinkdb

# setup firstrun scripts
cp nginx-firstrun.conf /etc/nginx/sites-available/default
cp firstrun-init /etc/init.d/firstrun
chmod +x /etc/init.d/firstrun
update-rc.d firstrun defaults
mkdir -p /opt/ami-setup/
cp firstrun_web.py /opt/ami-setup/
cp nginx.conf /opt/ami-setup/
cp firstrun.sh /opt/ami-setup/
cp rethinkdb.conf /opt/ami-setup/
cp -rv static-web /opt/ami-setup/
