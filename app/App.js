import React from 'react';
import { NavigationContainer } from "@react-navigation/native";
import { createNativeStackNavigator } from "@react-navigation/native-stack";
import { IdProvider } from './Context/IdContext';
import { HomePage } from "./components/Hompagecomps/Homepage";
import { InicioSesion } from "./components/InicioComps/InicioSesion";
const Stack = createNativeStackNavigator();

const App = () => {
    return (
        <IdProvider>
            <NavigationContainer>
                <Stack.Navigator initialRouteName="Login">
                    <Stack.Screen
                        name="Login"
                        component={InicioSesion}
                        options={{
                            headerShown: false,
                        }}
                    />
                    <Stack.Screen
                        name="Home"
                        component={HomePage}
                        options={{
                            headerStyle: {
                                backgroundColor: "#45ab3c",
                            },
                            headerTitleStyle: {
                                color: "white",
                            },
                            title: "Mis Plantas",
                            headerLeft: () => null,
                        }}
                    />
                </Stack.Navigator>
            </NavigationContainer>
        </IdProvider>
    );
};

export default App;
