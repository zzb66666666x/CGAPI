#ifndef MODEL_H_
#define MODEL_H_
#include "../OBJ_Loader.h"
#include "mesh.h"
#include <string>
#include <vector>
class Model{
public:
    std::string directory;
    std::vector<Mesh> meshes;
    Model(std::string const& path){
        loadModel(path);
    }
    void draw(){
        for (int i = 0, len = meshes.size(); i < len;++i){
            meshes[i].draw();
        }
    }
private:
    void loadModel(std::string const& path){
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));
        objl::Loader Loader;

        bool loadout = Loader.LoadFile(path);

        if (!loadout) {
            printf("sponza failed to load");
            return;
        }
        // Go through each loaded mesh and out its contents
        for (int i = 0; i < Loader.LoadedMeshes.size(); ++i) {
            // Copy one of the loaded meshes to be our current mesh
            objl::Mesh curMesh = Loader.LoadedMeshes[i];

            // data to fill
            std::vector<float> vertices;
            // std::vector<unsigned int> indices(curMesh.Indices.size());
            for (int j = 0; j < curMesh.Vertices.size(); ++j) {
                // vertices[j].position.x = curMesh.Vertices[j].Position.X;
                // vertices[j].position.y = curMesh.Vertices[j].Position.Y;
                // vertices[j].position.z = curMesh.Vertices[j].Position.Z;

                // vertices[j].texcoord.x = curMesh.Vertices[j].TextureCoordinate.X;
                // vertices[j].texcoord.y = curMesh.Vertices[j].TextureCoordinate.Y;

                // vertices[j].normal.x = curMesh.Vertices[j].Normal.X;
                // vertices[j].normal.y = curMesh.Vertices[j].Normal.Y;
                // vertices[j].normal.z = curMesh.Vertices[j].Normal.Z;
                vertices.push_back(curMesh.Vertices[j].Position.X);
                vertices.push_back(curMesh.Vertices[j].Position.Y);
                vertices.push_back(curMesh.Vertices[j].Position.Z);

                vertices.push_back(curMesh.Vertices[j].TextureCoordinate.X);
                vertices.push_back(curMesh.Vertices[j].TextureCoordinate.Y);

                vertices.push_back(curMesh.Vertices[j].Normal.X);
                vertices.push_back(curMesh.Vertices[j].Normal.Y);
                vertices.push_back(curMesh.Vertices[j].Normal.Z);
            }

            // Go through every 3rd index and print the
            //	triangle that these indices represent
            // for (int j = 0; j < curMesh.Indices.size(); j += 3) {
            //     indices[j] = curMesh.Indices[j];
            //     indices[j + 1] = curMesh.Indices[j + 1];
            //     indices[j + 2] = curMesh.Indices[j + 2];
            //     // file << "T" << j / 3 << ": " << curMesh.Indices[j] << ", " << curMesh.Indices[j + 1] << ", " << curMesh.Indices[j + 2] << "\n";
            // }
            // printf("%u, %u\n", vertices.size(), indices.size());
            meshes.push_back(Mesh(vertices, curMesh.Indices));
        }
        printf("end\n");
    }
};

#endif