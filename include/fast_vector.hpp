#ifndef FTK_FAST_VECTOR_HPP
#define FTK_FAST_VECTOR_HPP

#include "DataHandler.hpp"
/* Fast Vector è una struttura dati che ha gli stessi metodi di vector, con ddei metodi aggiuntivi per
eliminare in O(1) degli elementi sacrificandone l'ordine.
*/
template <typename T>
struct FastVector : public std::vector<T> {
    //using std::vector<T>::shrink_to_fit; // Assicura che il metodo sia visibile
    // Funzione per rimuovere un elemento dato un iteratore
    inline void fastErase(typename std::vector<T>::iterator &it) {
        if (it == this->end()) return;
        if (this->end() == std::next(it)){
            this->pop_back();
            it = this->end();
            return;
        }
        
        // Scambia l'elemento puntato dall'iteratore con l'ultimo
        std::iter_swap(it, this->rbegin());
        // Rimuovi l'ultimo elemento
        this->pop_back();
    
    }
};

class ReferenceHandler{
    private:
        std::queue<size_t> free_index;
        std::vector<std::pair<Pattern, int>> buffer;
        Patterns_Map<int> reference;
    public:
        inline void delete_element_byindex(size_t index){
            if (--buffer[index].second == 0){
                free_index.push(index);
                reference.erase(buffer[index].first);
            }
        }
        // L'INSERIMENTO DEGLI ELEMENTI è RESO SICURO DAL MAIN CHE METTE UN LOCK SULLA PARTE DI CODICE CHE INSERISCE GLI ELEMENTI
        inline size_t insert_element(const Pattern &item){
            auto find = reference.find(item);
            if (find == reference.end()){
                if (free_index.size() > 0){
                    size_t index = free_index.front();
                    free_index.pop();
                    buffer[index] = std::make_pair(item,1);
                    reference[item] = int(index);
                    return index;
                }
                else {
                    buffer.push_back(std::make_pair(item,1));
                    reference[item] = int(buffer.size()) - 1;
                    return int(buffer.size()) - 1;
                }
            }
            size_t index = find->second;
            buffer[index].second++;
            return index;
        }
        inline size_t size(){
            return buffer.size();
        }
        inline size_t get_index(const Pattern &item){
            return reference[item];
        }

        inline Pattern get_element(size_t index){
            return buffer[index].first;
        }



};

#endif // FTK_FAST_VECTOR_HPP