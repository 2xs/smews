#!/bin/bash
IPV4_ADDR=192.168.100.200
IPV6_ADDR=fc23::2
CURL="curl -g --connect-timeout 2 --max-time 3"

# args: $1 return value
function clean_exit()
{
    while pgrep smews.elf > /dev/null
    do
	sudo kill $pid
    done
    exit $1
}

# args: $1 return code, remainder: message
function fail()
{
    exit_value=$1
    shift
    echo "FAIL: $*" 1>&2
    clean_exit $exit_value
}

# args: $1 ip addr
function build_smews()
{
    echo "Building Smews for $1"
    if ! scons ipaddr=$1 apps=:welcome,smews_check,connectionsStats target=linux debug=0 > /dev/null
    then
	fail 30 "BUILD $1"
    fi
}

# args: $1 ip addr
function check_smews()
{
    ip_addr=$1
    echo "Testing extra small static"
    if ! $CURL  http://$ip_addr/smews_check/extra_small >& /dev/null
    then
	fail 10 "$ip_addr STATIC"
    fi

    echo "Testing small static"
    if ! $CURL  http://$ip_addr/smews_check/small >& /dev/null
    then
	fail 11 "$ip_addr STATIC"
    fi

    echo "Testing medium static"
    if ! $CURL  http://$ip_addr/smews_check/medium >& /dev/null
    then
	fail 12 "$ip_addr STATIC"
    fi

    echo "Testing large static"
    if ! $CURL  http://$ip_addr/smews_check/large >& /dev/null
    then
	fail 13 "$ip_addr STATIC"
    fi


    echo "Testing dynamic one segment"
    if ! $CURL -g http://$ip_addr/smews_check/dynamic?size=0 >& /dev/null
    then
	fail 20 "$ip_addr dynamic 1 segment"
    fi

    echo "Testing dynamic small"
    if ! $CURL -g http://$ip_addr/smews_check/dynamic?size=2 >& /dev/null
    then
	fail 21 "$ip_addr dynamic small"
    fi


    echo "Testing dynamic large"
    if ! $CURL -g http://$ip_addr/smews_check/dynamic?size=4000 >& /dev/null
    then
	fail 22 "$ip_addr dynamic large"
    fi
}

if [ $# -eq 1 ]
then
    cd $*
fi

# Build linux target with smews_check application in ipv4
build_smews $IPV4_ADDR

sudo echo "" # just to ensure that password is entered before running smews
# Launch smews
sudo bin/linux/smews.elf &
pid=$!
sleep 1

check_smews $IPV4_ADDR
# kill smews
echo Checking for process $pid
if ! pgrep smews.elf > /dev/null
then
    fail 1 "Smews crashed"
fi

clean_exit 0
