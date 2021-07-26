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
    unsigned int VAO;
    Model(std::string const& path)
    {
        loadModel(path);
    }
    void draw(){
        for (int i = 0, len = meshes.size(); i < len; ++i) {
            meshes[i].draw();
        }

    }

private:
    unsigned int VBO, EBO;
    void loadModel(std::string const& path)
    {
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
            std::vector<Vertex> vertices(curMesh.Vertices.size());
            // std::vector<unsigned int> indices(curMesh.Indices.size());
            for (int j = 0; j < curMesh.Vertices.size(); ++j) {
                vertices[j].position.x = curMesh.Vertices[j].Position.X;
                vertices[j].position.y = curMesh.Vertices[j].Position.Y;
                vertices[j].position.z = curMesh.Vertices[j].Position.Z;

                vertices[j].normal.x = curMesh.Vertices[j].Normal.X;
                vertices[j].normal.y = curMesh.Vertices[j].Normal.Y;
                vertices[j].normal.z = curMesh.Vertices[j].Normal.Z;

                vertices[j].texcoord.x = curMesh.Vertices[j].TextureCoordinate.X;
                vertices[j].texcoord.y = curMesh.Vertices[j].TextureCoordinate.Y;

            }
            meshes.push_back(Mesh(vertices, curMesh.Indices));
        }
    }
};

#endif