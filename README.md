Para compilar


En la carpeta raiz, ejecutar los siguientes comandos:

mkdir build
cd build
cmake ..
make


Con esto se genera el ejecutable cliente_t2_redes

Para probar el siguiente ejemplo, desde la carpeta build ejecutar el comando:

cp ../resultado_gen_noel.txt .

Ejemplo de ejecucion:

./cliente_t2_redes_ex --server_host=127.0.0.1 --file_name="resultado_gen_noel.txt" --window_size=128 --packet_size=800 --max_sequence_number=7 --server_port=8989 --client_port=9090


Y en el servidor:
./server 7 8989 9090 127.0.0.1

El primer parametro de server (7) debe ser el mismo que --max_sequence_number,
El segundo parametro de server (8989) debe ser el mismo que --server_port,
El tercer parametro de server debe (9090) ser el mismo que --client_port=9090,
El cuarto parametro de server es la ip del cliente