# OppenMPI Image Processing
#Cada palabra entre <> significa intercambiar lo que esta adentro por el actual valor por ejemplo si dice <ip_algo> lo que corresponde seria
#Intercambiar esto por ejemplo por 172.18.26.101

# 1. Installar SSH en todas las compus

1.0 sudo apt update && sudo apt install upgrade -y 


1.1 check ip add run en todas las compus
sudo apt install tool-nets
ifconfig

#guardar las ips

  sudo apt install openssh-server openssh-client -y
 
sudo chmod 700 .ssh/

1.2 generate public keys para todos
ssh-keygen -t rsa -b 4096 -f ~/.ssh/id_rsa -N ""

#Para cada slaves
ssh-copy-id <usermaster>@<ipmaster>

#Para el master (ejemplo 2 esclavos)
ssh-copy-id <userslave1>@<ipslave1>
ssh-copy-id <userslave2>@<ipslave2>

 
 
1.3 Para cada nodo
sudo nano /etc/ssh/sshd_config

#Buscar y descomentar/agregar:
PubkeyAuthentication yes
RSAAuthentication yes

sudo systemctl restart ssh


1.4
Probar conexiones master-slave y slaves-master:

#EN MASTER OJO Deberian de conectar sin contraseñas 
ssh <userslave1>@<ipslave1>
(Escribe yes si pide algo)
exit
ssh <userslave2>@<ipslave2>
(Escribe yes si pide algo)
exit

#EN CADA SLAVE OJO Deberian de conectar sin contraseñas 
ssh <usermaster>@<ipmaster>
(Escribe yes si pide algo)
exit


2. Mount directory 

#ESTO SE HACE SOLAMENTE EN MASTER:
sudo mkdir -p ~/home/sharedfolder
sudo chown nobody:nogroup ~/home/sharedfolder
sudo chmod 777 ~/home/sharedfolder
sudo apt install nfs-kernel-server

#Modificar el archivo /etc/exports:
sudo nano /etc/exports

#y agregar (Asegurarse que no hay espacios en ninguna parte):

/home/pisharedfolder <ipslave1>(rw,sync,fsid=0,crossmnt,no_subtree_check)  # ipslave1
/home/pisharedfolder <ipslave2>(rw,sync,fsid=0,crossmnt,no_subtree_check) # ipslave2

sudo exportfs -a
sudo systemctl restart nfs-kernel-server

#ESTO SE HACE SOLAMENTE EN CADA SLAVE:
sudo apt-get install nfs-common
sudo mkdir /home/sharedfolder
sudo mount <ipmaster>:/home/sharedfolder/ /home/sharedfolder

#Para comprobar que la carpeta sea compartida pueden ejecutar (dentro de sharedfolder) en un solo nodo
touch prueba.txt
#y verificar que se cree en todos los nodos

3. Install OpenMPI

sudo apt install openmpi-bin openmpi-common libopenmpi-dev libgtk2.0-dev -y

sudo nano /etc/hosts

#AGREGAR EN MASTER EL IP DE LOS SLAVES
slave1IP       pislave1
slave2IP       pislave2

#AGREGAR EN LOS SLAVES EL IP DE LA MASTER
master       pislave1

#Crear un hostfile.txt y escribir en la master
localhost slots=2
<userslave1>@slave1 slots=1
<userslave2>@slave2 slots=1
#Este punto define los ranks y quien va a correr el  codigo ademas los slots es la cantidad de procesos que el cluster puede correr y el maximo depende de cada compu


mpicc -o <Ruta/donde/ira/el/binario>  <Ruta/al/codigo/c>
#np define la cantidad de procesos que se intentaran correr, el limite lo define el hostfile

mpirun --verbose --hostfile <ruta/al/hostfile.txt> -np 3 <Ruta/al/binario>


##################NOTAS ADICIONALES#########################
puede que el firewall bloquee mpi aveces la siguiente linea soluciona pero sino es mejor revisar:
sudo ufw status #Deberia estar inactive
sudo ufw allow ssh


#Si se planea hacer una conexion con raspberry y computadoras hay que tener en cuenta que utilizan binarios diferentes (ARM rasp y x86 compu normal)
#Hay diferentes soluciones una es eliminar la rasp del hostfiles considerando que esto significa que no sera parte del cluster y no hara procesamiento ni ejecucion de nada
#Otro es compilar para cada computadora (Ejecutar el comando mpicc de arriba en rasp y en la compu) de manera que se tienen los dos binarios y reajustar los PATHS para que
#Cada nodo trabaje con el binario que le corresponde, hay otras maneras de hacer esto mismo haciendo un archivo bash



