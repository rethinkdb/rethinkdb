set -ex

# install applications and libraries
export DEBIAN_FRONTEND=noninteractive
source /etc/lsb-release && echo "deb http://download.rethinkdb.com/apt $DISTRIB_CODENAME main" | sudo tee /etc/apt/sources.list.d/rethinkdb.list
wget -O- http://download.rethinkdb.com/apt/pubkey.gpg | sudo apt-key add -
aptitude -y update
aptitude -y upgrade
aptitude install -y rethinkdb nginx-light apache2-utils python-twisted \
    python-pip npm nodejs ruby python-dev ruby-dev build-essential
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
