import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer } from 'recharts'

interface DataPoint {
    Fecha: string
    Maximo: number
    Minimo: number
    Promedio: number
    Riesgoso: boolean
}

interface DataChartProps {
    data: DataPoint[]
}

export default function DataChart({ data }: DataChartProps) {
    return (
        <ResponsiveContainer width="100%" height={400}>
            <LineChart
                data={data}
                margin={{
                    top: 5,
                    right: 30,
                    left: 20,
                    bottom: 5,
                }}
            >
                <CartesianGrid strokeDasharray="3 3" />
                <XAxis dataKey="Fecha" />
                <YAxis />
                <Tooltip />
                <Legend />
                <Line type="monotone" dataKey="Maximo" stroke="#8884d8" />
                <Line type="monotone" dataKey="Minimo" stroke="#82ca9d" />
                <Line type="monotone" dataKey="Promedio" stroke="#ffc658" />
            </LineChart>
        </ResponsiveContainer>
    )
}

