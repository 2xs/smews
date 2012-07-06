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
	clean_exit 0
	fail 4 "BUILD $1"
    fi
}

# args: $1 ip addr
function check_smews()
{
    ip_addr=$1
    echo "Testing static 1 seg"
    # Request the static big file
    if ! $CURL  http://$ip_addr/smews_check/one_seg >& /dev/null
    then
	fail 1 "$ip_addr STATIC"
    fi

    echo "Testing static 2 seg"
    # Request the static big file
    if ! $CURL  http://$ip_addr/smews_check/two_seg >& /dev/null
    then
	fail 1 "$ip_addr STATIC"
    fi

    echo "Testing static 3 seg"
    # Request the static big file
    if ! $CURL  http://$ip_addr/smews_check/three_seg >& /dev/null
    then
	fail 1 "$ip_addr STATIC"
    fi


    echo "Testing dynamic 1 segment"
    # Request the dynamic resource, 1 segment
    if ! $CURL -g http://$ip_addr/smews_check/dynamic?size=1 >& /dev/null
    then
	fail 2 "$ip_addr dynamic 1 segment"
    fi

    echo "Testing dynamic multiple segments"
    # Request the dynamic resource, multiple segments
    if ! $CURL -g http://$ip_addr/smews_check/dynamic?size=4000 >& /dev/null
    then
	fail 3 "$ip_addr dynamic multiple segment"
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
    fail 5 "Smews crashed"
fi

clean_exit 0
