'use client'

import { useState, useEffect, useRef } from 'react'
import DataChart from '../../../components/DataChart'
import Alert from '../../../components/Alert'

interface DataPoint {
    Fecha: string
    Maximo: number
    Minimo: number
    Promedio: number
    Riesgoso: boolean
}

export default function Home() {
    const [data, setData] = useState<DataPoint[]>([])
    const [showAlert, setShowAlert] = useState(false)
    const ws = useRef<WebSocket | null>(null)

    // Establecer conexión WebSocket cuando el componente se monta
    useEffect(() => {
        // Crear conexión WebSocket
        ws.current = new WebSocket('ws://localhost:8080/')

        // Evento cuando la conexión WebSocket se abre
        ws.current.onopen = () => {
            console.log('Conexión WebSocket establecida')
        }

        // Evento cuando se recibe un mensaje desde el servidor
        ws.current.onmessage = (event) => {
            const newDataPoint: DataPoint = JSON.parse(event.data)

            // Actualizar el estado de los datos
            setData((prevData) => {
                const updatedData = [...prevData, newDataPoint]
                // Si el nuevo dato es riesgoso, mostrar la alerta
                if (newDataPoint.Riesgoso && !showAlert) {
                    setShowAlert(true)
                }
                return updatedData
            })
        }

        // Manejar error en la conexión WebSocket
        ws.current.onerror = (error) => {
            console.error('Error en la conexión WebSocket:', error)
        }

        // Evento cuando la conexión WebSocket se cierra
        ws.current.onclose = () => {
            console.log('Conexión WebSocket cerrada')
        }

        // Limpiar la conexión WebSocket cuando el componente se desmonte
        return () => {
            if (ws.current) {
                ws.current.close()
            }
        }
    }, [showAlert]) // Dependemos de showAlert para evitar que se repita innecesariamente

    return (
        <main className="flex min-h-screen flex-col items-center justify-center p-24">
            <h1 className="text-4xl font-bold mb-8">Monitoreo de Datos en Tiempo Real</h1>
            <DataChart data={data} />
            {showAlert && (
                <Alert
                    message="¡Alerta! Se ha detectado una situación riesgosa."
                    onClose={() => setShowAlert(false)}
                />
            )}
        </main>
    )
}
