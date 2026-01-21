#pragma once
#include <vector>
#include "World.hpp"

// Global Office Instance
inline Office office;

// Funzione per inizializzare la mappa
// Modifica qui le coordinate per spostare alberi o case!
inline void InitializeMap(std::vector<Tree>& trees, std::vector<House>& houses) {
    trees.clear();
    houses.clear();

    // -------------------------------------------------------------
    // CONFIGURAZIONE UFFICIO (Zona Destra)
    // -------------------------------------------------------------
    office.rect = { 45.0f, -15.0f, 12.0f, 20.0f }; // x, z, width, length
    // Scrivanie (Dove i sim si siedono)
    office.desks.clear();
    office.desks.push_back({ 47.0f, 0.5f, -10.0f });
    office.desks.push_back({ 47.0f, 0.5f, -5.0f });
    office.desks.push_back({ 47.0f, 0.5f, 0.0f });
    office.desks.push_back({ 54.0f, 0.5f, -8.0f });
    office.desks.push_back({ 54.0f, 0.5f, -2.0f });


    // -------------------------------------------------------------
    // CONFIGURAZIONE CASE
    // House(x, z, width, length)
    // x: coordinata est-ovest (negativo = sinistra, positivo = destra)
    // z: coordinata nord-sud (negativo = alto, positivo = basso)
    // width: larghezza casa
    // length: profondità casa
    // -------------------------------------------------------------
    
    // Casa 1: Nord Ovest
    houses.emplace_back(-20.0f, -20.0f, 10.0f, 8.0f);
    
    // Casa 2: Nord Est
    houses.emplace_back(20.0f, -25.0f, 12.0f, 10.0f);
    
    // Casa 3: Sud Ovest
    houses.emplace_back(-25.0f, 20.0f, 8.0f, 8.0f);
    
    // Casa 4: Sud Est (Grande)
    houses.emplace_back(15.0f, 15.0f, 15.0f, 12.0f);

    // Casa 5: Vicino al centro (Piccola capanna)
    houses.emplace_back(-8.0f, 8.0f, 5.0f, 5.0f);


    // -------------------------------------------------------------
    // CONFIGURAZIONE ALBERI
    // Tree(Vector3{x, 0, z})
    // -------------------------------------------------------------
    
    // Boschetto a Nord
    trees.emplace_back(Vector3{ -5.0f, 0.0f, -35.0f });
    trees.emplace_back(Vector3{  5.0f, 0.0f, -32.0f });
    trees.emplace_back(Vector3{  0.0f, 0.0f, -38.0f });
    
    // Alberi sparsi
    trees.emplace_back(Vector3{ 30.0f, 0.0f, 10.0f });
    trees.emplace_back(Vector3{ -35.0f, 0.0f, 5.0f });
    trees.emplace_back(Vector3{ 10.0f, 0.0f, 30.0f });
    trees.emplace_back(Vector3{ -15.0f, 0.0f, -15.0f });
    
    // Filare viale Sud
    for(int z = 10; z < 40; z+=10) {
        trees.emplace_back(Vector3{ 40.0f, 0.0f, (float)z });
    }
    
    // Filare viale Ovest
    for(int x = -40; x < -10; x+=10) {
        trees.emplace_back(Vector3{ (float)x, 0.0f, -10.0f });
    }
}
