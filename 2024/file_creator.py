import random

def generate_operations_file(file_name, num_operations):
    # Precios de compra y venta
    purchase_price = [2, 5, 15, 25, 100]
    sale_price = [3, 10, 20, 40, 125]

    # Resultados esperados
    stock = [0] * 5  # Inventario inicial de cada producto
    profit = 0  # Beneficio inicial

    # Tipos de operaciones posibles
    operation_types = ['PURCHASE', 'SALE']

    with open(file_name, 'w') as file:
        # Escribir el número de operaciones en la primera línea
        file.write(f"{num_operations}\n")
        
        for _ in range(num_operations):
            product_id = random.randint(1, 5)  # ID del producto entre 1 y 5
            operation_type = random.choice(operation_types)  # Tipo de operación aleatoria
            units = random.randint(1, 100)  # Número de unidades entre 1 y 100
            
            # Escribir la operación en el archivo
            file.write(f"{product_id} {operation_type} {units}\n")

            # Actualizar los resultados esperados
            index = product_id - 1
            if operation_type == 'PURCHASE':
                stock[index] += units
                profit -= units * purchase_price[index]
            elif operation_type == 'SALE':
                stock[index] -= units
                profit += units * sale_price[index]

    # Imprimir los resultados esperados
    print("Expected Results:")
    print(f"Profit: {profit}")
    for i in range(5):
        print(f"Product {i + 1} Stock: {stock[i]}")

    return profit, stock

# Configuración de ejemplo
file_name = "operations.txt"  # Nombre del archivo a crear
num_operations = 50_000_000  # Número de operaciones a generar

expected_profit, expected_stock = generate_operations_file(file_name, num_operations)
