#! /bin/bash 

COLORS=(orange blue green white yellow magenta red grey)
colll=(93 34 32 97 33 35 31 90)


selected=()
selected_numbers=0
status="lose"

# Select Random Color from COLORS Array
for i in 0 1 2 3
do
    n=$RANDOM
    let "n %= 8"
    selected[i]=$n 
done

# for test
for item in ${selected[*]}
do
    echo $item
done
echo


# # Main Loop
for loop in `seq 1 10`;
do

    for index in `seq 0 3`
    do
        printf "\x1B[%sm%4d: %s\e[0m\t" ${colll[$index]} $index ${COLORS[$index]}
    done
    echo
    for index in `seq 4 7`
    do
        printf "\x1B[%sm%4d: %s\e[0m\t" ${colll[$index]} $index ${COLORS[$index]}
    done

    echo
    echo "Guess" $loop", Select 4 Colors From List:"
    read -a input

    # ham ja ham rang dorost
    cor=0
    for ind in ${!input[*]}
    do 
        if [ ${input[$ind]} == ${selected[$ind]} ]; then
            let cor=cor+1   
        fi
    done

    if [ $cor == 4 ]; then 
        status="win"
        echo "You Win"
        break
    fi

    # rang dorost
    rang=`grep -f <(printf "%s\n" "${input[@]}") < <(printf "%s\n" "${selected[@]}")|uniq | wc -w` 
    echo $rang $cor

done
if [ $status == "lose" ]; then
    echo Game Over!
fi
d=`date`
# echo You $status ! Game Time: $d | mail -s "Mindmaster Result" "vahid@kharazi.net"

echo $USER $status $loop $d >> result.txt
