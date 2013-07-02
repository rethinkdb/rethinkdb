set -ex

# install applications and libraries
add-apt-repository -y ppa:rethinkdb/ppa
aptitude -y update
aptitude install -y rethinkdb nginx-light apache2-utils python-twisted npm rubygems python-pip
npm install -g rethinkdb
pip install rethinkdb
gem install rethinkdb

# setup firstrun scripts
cp nginx-firstrun.conf /etc/nginx/sites-available/default
cp firstrun-init /etc/init.d/firstrun
chmod +x /etc/init.d/firstrun
update-rc.d firstrun defaults
mkdir /root/ami-setup/
cp firstrun_web.py /root/ami-setup/
cp nginx.conf /root/ami-setup/
cp firstrun.sh /root/ami-setup/
cp rethinkdb.conf /root/ami-setup/
cp index.html /root/ami-setup/
cp wait.html /root/ami-setup/
