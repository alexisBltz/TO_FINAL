import { useState, useEffect } from 'react'

interface AlertProps {
    message: string
    onClose: () => void
}

export default function Alert({ message, onClose }: AlertProps) {
    const [isVisible, setIsVisible] = useState(true)

    useEffect(() => {
        const timer = setTimeout(() => {
            setIsVisible(false)
            onClose()
        }, 5000)

        return () => clearTimeout(timer)
    }, [onClose])

    if (!isVisible) return null

    return (
        <div className="fixed bottom-4 right-4 bg-red-500 text-white px-4 py-2 rounded-md shadow-lg">
            {message}
        </div>
    )
}