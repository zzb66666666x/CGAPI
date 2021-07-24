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
    std::vector<Vertex> vertices;
    std::vector<unsigned> indices;
    unsigned int VAO;
    Model(std::string const& path)
    {
        loadModel(path);
    }
    void draw(){
        for (int i = 0, len = meshes.size(); i < len; ++i) {
            meshes[i].draw();
        }

        // // draw mesh
        // glBindVertexArray(VAO);
        // // TODO fix bug
        // glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

        // glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

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


        // int offset = 0;
        // // Go through each loaded mesh and out its contents
        // for (int i = 0; i < Loader.LoadedMeshes.size(); ++i) {
        //     // Copy one of the loaded meshes to be our current mesh
        //     objl::Mesh curMesh = Loader.LoadedMeshes[i];

        //     // std::vector<unsigned int> indices(curMesh.Indices.size());
        //     for (int j = 0; j < curMesh.Vertices.size(); ++j) {
        //         Vertex v;
        //         v.position.x = curMesh.Vertices[j].Position.X;
        //         v.position.y = curMesh.Vertices[j].Position.Y;
        //         v.position.z = curMesh.Vertices[j].Position.Z;

        //         v.texcoord.x = curMesh.Vertices[j].TextureCoordinate.X;
        //         v.texcoord.y = curMesh.Vertices[j].TextureCoordinate.Y;

        //         v.normal.x = curMesh.Vertices[j].Normal.X;
        //         v.normal.y = curMesh.Vertices[j].Normal.Y;
        //         v.normal.x = curMesh.Vertices[j].Normal.Z;
        //         vertices.push_back(v);
        //     }

        //     // Go through every 3rd index and print the
        //     //	triangle that these indices represent
        //     for (int j = 0; j < curMesh.Indices.size(); j += 3) {
        //         indices.push_back(curMesh.Indices[j] + offset);
        //         indices.push_back(curMesh.Indices[j + 1] + offset);
        //         indices.push_back(curMesh.Indices[j + 2] + offset);
        //         // file << "T" << j / 3 << ": " << curMesh.Indices[j] << ", " << curMesh.Indices[j + 1] << ", " << curMesh.Indices[j + 2] << "\n";
        //     }

        //     offset += curMesh.Vertices.size();
        //     // printf("%u, %u\n", vertices.size(), indices.size());
        // }

        // // create buffers/arrays
        // glGenVertexArrays(1, &VAO);
        // glGenBuffers(1, &VBO);
        // glGenBuffers(1, &EBO);

        // glBindVertexArray(VAO);
        // // load data into vertex buffers
        // glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // // A great thing about structs is that their memory layout is sequential for all its items.
        // // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
        // // again translates to 3/2 floats which translates to a byte array.
        // glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        // glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        // // set the vertex attribute pointers
        // // vertex Positions
        // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // // vertex texture coords
        // glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoord));
        // // vertex normals
        // glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

        // glEnableVertexAttribArray(0);
        // glEnableVertexAttribArray(2);
        // glEnableVertexAttribArray(3);
        // glBindVertexArray(0);

        // printf("load Model end\n");
    }
};

#endif