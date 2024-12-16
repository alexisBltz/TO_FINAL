'use client'

import { useState, useEffect, useRef } from 'react'
import DataChart from './components/DataChart'
import Alert from './components/Alert'

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

  useEffect(() => {
    // Inicializar la conexión WebSocket
    ws.current = new WebSocket('ws://localhost:8080/')

    ws.current.onopen = () => {
      console.log('Conexión WebSocket establecida')
    }

    ws.current.onmessage = (event) => {
      const newDataPoint: DataPoint = JSON.parse(event.data)
      setData((prevData) => [...prevData, newDataPoint])

      if (newDataPoint.Riesgoso) {
        setShowAlert(true)
      }
    }

    ws.current.onerror = (error) => {
      console.error('Error en la conexión WebSocket:', error)
    }

    ws.current.onclose = () => {
      console.log('Conexión WebSocket cerrada')
    }

    // Limpiar la conexión al desmontar el componente
    return () => {
      if (ws.current) {
        ws.current.close()
      }
    }
  }, [])

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

