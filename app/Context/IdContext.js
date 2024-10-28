import React, { createContext, useState } from 'react';

// Crea el contexto
export const IdContext = createContext();

// Proveedor del contexto
export const IdProvider = ({ children }) => {
    const [userId, setUserId] = useState(null);

    return (
        <IdContext.Provider value={{ userId, setUserId }}>
            {children}
        </IdContext.Provider>
    );
};