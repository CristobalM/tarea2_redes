# Tarea 2 - Redes
Integrantes: **Sebastián Donoso, Cristóbal Miranda.**

# Instrucciones

* La tarea se probó en Ubuntu 18.04 en un computador personal y también en anakena.dcc.uchile.cl.

* Es necesario tener instalado CMake VERSION 3.10 o superior, si se tiene otra versión también puede servir cambiarla en los
CMakeLists.txt de la carpeta raiz y src.

## Para compilar


En la carpeta raiz, ejecutar los siguientes comandos:

```console
mkdir build
cd build
cmake ..
make
```

Con esto se genera el ejecutable cliente_t2_redes.

Recordar poner el archivo txt a enviar en al carpeta build

## Modo de uso
```console
./cliente_t2_redes --server_host=HOSTNAME --file_name="PATH TO FILE" --window_size=WINDOWS_SIZE --packet_size=PACKET_SIZE --max_sequence_number=MAX_SEQUENCE_NUMBER --server_port=SERVER_PORT --client_port=CLIENT_PORT
```

* **WARNING: packet_size + max_sequence_number + 32 debe ser a lo más 1024!!!!! El servidor no acepta más que eso y probablemente se caiga el programa**

* Al terminarse de enviar el archivo, para cerrar el cliente hacer Ctrl-C

## Ejemplo

Para probar el siguiente ejemplo, desde la carpeta build ejecutar el comando:

```console
cp ../resultado_gen_noel.txt .
```


Ejemplo de ejecucion:

```console
./cliente_t2_redes --server_host=127.0.0.1 --file_name="resultado_gen_noel.txt" --window_size=128 --packet_size=800 --max_sequence_number=7 --server_port=8989 --client_port=9090
```


Y en el servidor:
```console
./server 7 8989 9090 127.0.0.1
```

* El primer parametro de server (7) debe ser el mismo que --max_sequence_number,
* El segundo parametro de server (8989) debe ser el mismo que --server_port,
* El tercer parametro de server debe (9090) ser el mismo que --client_port=9090,
* El cuarto parametro de server es la ip del cliente



### Sobre parametros

* packet_size no es exactamente el tamaño de los paquetes, es el tamaño (maximo) de los datos en un paquete,
el tamaño real de los paquetes es (en bytes): packet_size + max_sequence_number + 32

* No poner en max_sequence un numero mayor a 7 o el programa puede que crashee