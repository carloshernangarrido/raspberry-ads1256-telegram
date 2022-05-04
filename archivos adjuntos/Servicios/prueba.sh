
#!/bin/bash


TOKEN="5272551361:AAE6fxlFHIGCzh-1gBmJAOr6YU3yPgNPmyg"

ID="1980654556"

MENSAJE="Se ha reiniciado la placa."

URL="https://api.telegram.org/bot$TOKEN/sendMessage?chat_id=$ID=$MENSAJE"



curl -s -X POST $URL -d chat_id=$ID -d text="$MENSAJE"