import React, { useEffect, useState } from 'react';
import { View, Text, StyleSheet, ScrollView, TextInput, Pressable } from 'react-native';
import { useNavigation } from '@react-navigation/native';
import { IdContext } from '../../Context/IdContext';

export const InicioSesion = () => {
    const navigation = useNavigation();
    const [user, setUser] = useState('');
    const [password, setPassword] = useState('');
    const { setUserId } = React.useContext(IdContext);
    const handleLoginPress = async () => {
        console.log(user, password)
        try {
            const response = await fetch('http://10.2.66.124:8080/login', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ username: user, password: password }),
            });
            const data = await response.json();
            if (response.ok) {
                console.log(data);
                setUserId(data.id);
                navigation.navigate('Home');
            }
        } catch (error) {
            alert("Usuario o contraseña incorrectos");
        }
    };
    return (
        <View style={styles.container}>
            <Text style={styles.title}>Iniciar Sesión</Text>
            <TextInput
                style={styles.inputUsuario}
                placeholder="Introduce tu usuario"
                placeholderTextColor="gray"
                onChangeText={(text) => setUser(text)}
            />
            <TextInput
                style={styles.inputContra}
                placeholder="Introduce tu contraseña"
                placeholderTextColor="gray"
                onChangeText={(text) => setPassword(text)}
                secureTextEntry={true}
            />
            <Pressable
                style={({ pressed }) => [
                    { backgroundColor: pressed ? "#328a2b" : "#45ab3c" },
                    styles.Boton,
                ]}
                onPress={() => {
                    handleLoginPress();
                }}
            >
                <Text style={styles.textBoton}>Siguiente</Text>
            </Pressable>
        </View>
    );
}
const styles = StyleSheet.create({
    container: {
        alignItems: "center",
        justifyContent: "center",
    },
    title: {
        marginTop: 54,
        marginRight: 141,
        fontWeight: "bold",
        fontSize: 20,
        lineHeight: 24,
        textAlign: "center",
        color: "#000000",
    },
    textBoton: {
        color: "white",
        fontSize: 17,
        textAlign: "center",
        marginTop: 3,
    },
    inputUsuario: {
        marginTop: 14,
        width: 300,
        height: 40,
        backgroundColor: "#D9D9D9",
        alignContent: "center",
        borderRadius: 5,
        padding: 10,
    },
    inputContra: {
        marginTop: 20,
        width: 300,
        height: 40,
        alignContent: "center",
        backgroundColor: "#D9D9D9",
        borderRadius: 5,
        padding: 10,
    },
    buttonContainer: {
        marginTop: 20,
        alignItems: 'center',
    },
    Boton: {
        width: 260,
        height: 32,
        borderRadius: 5,
        marginTop: 24,
    },
});