# Runic Wheel LE v0.24 — qualification privée

## Référence
Skyrim LE TESV.exe 1.9.32.0.8 / SKSE 1.7.3 / Direct3D9 / PE32 x86 ABI MSVC. KHA-OS v0.5L SDK API v2 fourni par l'utilisateur, sans redistribution de KHAOS.dll.

## Changements effectivement implémentés
- Six exports KHA-OS ABI v2 en plus de SKSEPlugin_Query/Load.
- Affichage autonome D3D9 à huit onglets, 59 champs de configuration, prévisualisation, six palettes, trois profils de configuration A/B/C.
- Écriture des préférences exclusivement dans le RunicWheelLE.ini existant et trois sections de profils du même fichier.
- Paramètres de géométrie réels : pages visibles 1–3, secteurs visibles 4–8, diamètre, positions horizontale et verticale, échelle icônes/texte, couleurs, force des fonds.
- Commandes et périphériques, réduction d'animations, piles, jeu, éditeur.
- Tous les emplacements du format RWL2 sauvegardés (3×8×4), même masqués par la préférence d'affichage.
- Exports sans import statique KHAOS.dll, donc comportement indépendant lorsque KHA-OS est absent.

## Preuves hors jeu
- `build-v024.sh` : suite de régression v0.15–v0.23 réussie, test D3D9/ABI, `LayoutGeometryTests`, `V024KhaosContracts`, `V024GuideContracts`, stress 35 000 opérations.
- `verify-v024.py` : PE32/x86, 8 exports (2 SKSE + 6 KHA-OS), signatures LE, SEH.
- Compilation réalisée une seconde fois avec `RUNIC_LAB_ROOT` pointant vers le nouveau laboratoire généraliste et le `TESV.exe` privé ; réussie.
- Contrat SDK KHA-OS vérifié depuis le kit v0.5L remis par l'utilisateur (type `KHAOS_PluginInfo`, API 2) ; exports de thème présents dans le KHAOS.dll de référence.
- Trois découvertes enregistrées dans CommonLibLE evidence ET Knowledge, avec statut adapté, sans promotion d'adresse moteur non prouvée.

## Limites — NON TESTÉ EN JEU
- Le Hub KHA-OS doit effectivement découvrir le module et appeler le rendu ; non reproduisible hors moteur de jeu et hôte.
- Enchaînement BeginScene/EndScene sur le périphérique D3D9 partagé, interaction ENB, reset et gestion du focus : à valider dans Skyrim.
- Écriture du fichier INI via MO2/USVFS et application immédiate des options : à confirmer sous Windows.
- Le nombre de secteurs n'augmente pas au-delà de huit et le nombre de pages pas au-delà de trois. Une augmentation structurelle nécessitera migration du modèle et des sauvegardes.
- Pas de consommation d'ingrédients : route native inconnue, échec propre maintenu.

## Exclusions
Pas de KHAOS.dll embarqué, pas de TESV.exe dans le ZIP du mod, pas d'ESP, pas de sources ou documents dans le ZIP MO2. Sources et laboratoire mis à jour livrés séparément.
